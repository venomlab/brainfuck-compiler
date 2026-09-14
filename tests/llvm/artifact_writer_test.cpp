#include <bfc/llvm/artifact_writer.hpp>
#include <bfc/llvm/assembly_writer.hpp>
#include <bfc/llvm/external_link_writer.hpp>
#include <bfc/llvm/ir_writer.hpp>
#include <bfc/llvm/object_writer.hpp>
#include <gtest/gtest.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace bfc::llvm {
namespace {

static_assert(std::is_abstract_v<ArtifactWriter>);
static_assert(std::is_base_of_v<ArtifactWriter, IRWriter>);
static_assert(std::is_base_of_v<ArtifactWriter, AssemblyWriter>);
static_assert(std::is_base_of_v<ArtifactWriter, ObjectWriter>);
static_assert(std::is_base_of_v<ArtifactWriter, ExternalLinkWriter>);

TEST(ArtifactWriterTest, OwnsEveryWriterThroughCommonInterface) {
    const ::llvm::Triple target(::llvm::sys::getDefaultTargetTriple());
    std::vector<std::unique_ptr<ArtifactWriter>> writers;

    writers.push_back(std::make_unique<IRWriter>());
    writers.push_back(std::make_unique<AssemblyWriter>(target));
    writers.push_back(std::make_unique<ObjectWriter>(target));
    writers.push_back(std::make_unique<ExternalLinkWriter>(target, std::make_unique<IRWriter>()));

    EXPECT_EQ(writers.size(), 4);
}

TEST(ArtifactWriterTest, ExposesFileExtensionsThroughCommonInterface) {
    const ::llvm::Triple target(::llvm::sys::getDefaultTargetTriple());
    std::vector<std::unique_ptr<ArtifactWriter>> writers;

    writers.push_back(std::make_unique<IRWriter>());
    writers.push_back(std::make_unique<AssemblyWriter>(target));
    writers.push_back(std::make_unique<ObjectWriter>(target));
    writers.push_back(std::make_unique<ExternalLinkWriter>(target, std::make_unique<IRWriter>()));

    EXPECT_EQ(writers[0]->file_ext(), ".ll");
    EXPECT_EQ(writers[1]->file_ext(), ".s");
    EXPECT_EQ(writers[2]->file_ext(), ".o");
    EXPECT_TRUE(writers[3]->file_ext().empty());
}

TEST(ArtifactWriterTest, DispatchesWriteThroughCommonInterface) {
    ::llvm::LLVMContext context;
    ::llvm::Module module("brainfuck", context);
    std::ostringstream output;
    const std::unique_ptr<ArtifactWriter> writer = std::make_unique<IRWriter>();

    writer->write(module, output);

    EXPECT_NE(output.str().find("; ModuleID = 'brainfuck'"), std::string::npos);
}

} // namespace
} // namespace bfc::llvm
