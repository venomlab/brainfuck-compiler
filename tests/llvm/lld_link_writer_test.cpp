#include <bfc/core/ast.hpp>
#include <bfc/llvm/executable_emission_exception.hpp>
#include <bfc/llvm/ir_generator.hpp>
#include <bfc/llvm/lld_link_writer.hpp>
#include <bfc/llvm/object_writer.hpp>
#include <bfc/llvm/runtime_generator.hpp>
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
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace bfc::llvm {
namespace {

std::set<std::filesystem::path> temporary_lld_files() {
    const std::string prefix = "brainfuck-compiler-" + std::to_string(::llvm::sys::Process::getProcessId()) + '-';
    std::set<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::temp_directory_path())) {
        if (entry.path().filename().string().starts_with(prefix)) {
            files.insert(entry.path());
        }
    }
    return files;
}

std::unique_ptr<::llvm::Module> make_module(::llvm::LLVMContext& context, const ::llvm::Triple& target) {
    const ast::Program program(std::make_unique<ast::Sequence>(std::vector<std::unique_ptr<ast::Node>> {}));
    auto module = std::make_unique<::llvm::Module>("brainfuck", context);
    IRGenerator generator(*module);
    generator.generate(program);
    RuntimeGenerator(target).generate(*module);
    return module;
}

void expect_static_executable(const std::string& executable_data) {
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
}

TEST(LLDLinkWriterTest, WritesStaticExecutable) {
    ::llvm::LLVMContext context;
    const ::llvm::Triple target("x86_64-unknown-linux-gnu");
    auto module = make_module(context, target);
    const LLDLinkWriter writer {ObjectWriter(target)};
    std::ostringstream output(std::ios::out | std::ios::binary);
    const auto temporary_files_before = temporary_lld_files();

    writer.write(*module, output);

    EXPECT_EQ(temporary_lld_files(), temporary_files_before);
    const std::string executable_data = output.str();
    expect_static_executable(executable_data);

    ::llvm::SmallString<128> executable_path;
    ASSERT_FALSE(::llvm::sys::fs::createTemporaryFile("brainfuck-lld-test", "out", executable_path));
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

TEST(LLDLinkWriterTest, SupportsRepeatedLinking) {
    ::llvm::LLVMContext context;
    const ::llvm::Triple target("x86_64-unknown-linux-gnu");
    auto module = make_module(context, target);
    const LLDLinkWriter writer {ObjectWriter(target)};
    std::ostringstream first_output(std::ios::out | std::ios::binary);
    std::ostringstream second_output(std::ios::out | std::ios::binary);

    writer.write(*module, first_output);
    writer.write(*module, second_output);

    expect_static_executable(first_output.str());
    expect_static_executable(second_output.str());
}

TEST(LLDLinkWriterTest, RejectsBrokenOutputStream) {
    ::llvm::LLVMContext context;
    const ::llvm::Triple target("x86_64-unknown-linux-gnu");
    auto module = make_module(context, target);
    const LLDLinkWriter writer {ObjectWriter(target)};
    std::ostringstream output;
    output.setstate(std::ios::badbit);

    EXPECT_THROW(writer.write(*module, output), ExecutableEmissionException);
}

} // namespace
} // namespace bfc::llvm
