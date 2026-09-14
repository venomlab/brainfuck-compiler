#include <bfc/core/ast.hpp>
#include <bfc/llvm/executable_emission_exception.hpp>
#include <bfc/llvm/executable_writer.hpp>
#include <bfc/llvm/ir_generator.hpp>
#include <bfc/llvm/object_writer.hpp>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <ios>
#include <llvm/ADT/SmallString.h>
#include <llvm/BinaryFormat/ELF.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Program.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace bfc::llvm {
namespace {

std::set<std::filesystem::path> temporary_compiler_files() {
    const std::string prefix = "brainfuck-compiler-" + std::to_string(::llvm::sys::Process::getProcessId()) + '-';
    std::set<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::temp_directory_path())) {
        if (entry.path().filename().string().starts_with(prefix)) {
            files.insert(entry.path());
        }
    }
    return files;
}

TEST(ExecutableWriterTest, WritesStaticExecutable) {
    const ast::Program program(std::make_unique<ast::Sequence>(std::vector<std::unique_ptr<ast::Node>> {}));
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);
    generator.generate(program);

    const ExecutableWriter writer {ObjectWriter(::llvm::Triple(::llvm::sys::getDefaultTargetTriple()))};
    std::ostringstream output(std::ios::out | std::ios::binary);
    const auto temporary_files_before = temporary_compiler_files();

    writer.write(module, output);

    EXPECT_EQ(temporary_compiler_files(), temporary_files_before);
    const std::string executable_data = output.str();
    ASSERT_FALSE(executable_data.empty());

    const ::llvm::MemoryBufferRef buffer(::llvm::StringRef(executable_data.data(), executable_data.size()),
                                         "brainfuck");
    auto executable = ::llvm::object::ObjectFile::createObjectFile(buffer);
    if (!executable) {
        FAIL() << ::llvm::toString(executable.takeError());
    }

    const auto* elf = ::llvm::dyn_cast<::llvm::object::ELF64LEObjectFile>(executable->get());
    ASSERT_NE(elf, nullptr);
    EXPECT_EQ(elf->getELFFile().getHeader().e_type, ::llvm::ELF::ET_EXEC);
    auto program_headers = elf->getELFFile().program_headers();
    if (!program_headers) {
        FAIL() << ::llvm::toString(program_headers.takeError());
    }
    for (const auto& header : *program_headers) {
        EXPECT_NE(header.p_type, ::llvm::ELF::PT_INTERP);
        EXPECT_NE(header.p_type, ::llvm::ELF::PT_DYNAMIC);
    }

    ::llvm::SmallString<128> executable_path;
    ASSERT_FALSE(::llvm::sys::fs::createTemporaryFile("brainfuck-executable-test", "out", executable_path));
    const ::llvm::FileRemover remove_executable(executable_path);
    {
        std::ofstream executable_output(executable_path.c_str(), std::ios::binary | std::ios::trunc);
        executable_output.write(executable_data.data(), static_cast<std::streamsize>(executable_data.size()));
        ASSERT_TRUE(executable_output);
    }
    ASSERT_FALSE(::llvm::sys::fs::setPermissions(
        executable_path, ::llvm::sys::fs::owner_all | ::llvm::sys::fs::group_read | ::llvm::sys::fs::group_exe |
                             ::llvm::sys::fs::others_read | ::llvm::sys::fs::others_exe));

    const ::llvm::SmallVector<::llvm::StringRef, 1> arguments {executable_path};
    EXPECT_EQ(::llvm::sys::ExecuteAndWait(executable_path, arguments), 0);
}

TEST(ExecutableWriterTest, RejectsBrokenOutputStream) {
    const ast::Program program(std::make_unique<ast::Sequence>(std::vector<std::unique_ptr<ast::Node>> {}));
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    IRGenerator generator(module);
    generator.generate(program);

    const ExecutableWriter writer {ObjectWriter(::llvm::Triple(::llvm::sys::getDefaultTargetTriple()))};
    std::ostringstream output;
    output.setstate(std::ios::badbit);

    EXPECT_THROW(writer.write(module, output), ExecutableEmissionException);
}

} // namespace
} // namespace bfc::llvm
