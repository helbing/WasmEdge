// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/types.h"
#include "processfunc.h"
#include "processmodule.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
WasmEdge::Runtime::Instance::ModuleInstance *createModule() {
  using namespace std::literals::string_view_literals;
  if (const auto *Plugin =
          WasmEdge::Plugin::Plugin::find("wasmedge_process"sv)) {
    if (const auto *Module = Plugin->findModule("wasmedge_process"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, uint32_t Cnt, uint8_t C = 0) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(Offset), Cnt, C);
}

void fillMemContent(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                    uint32_t Offset, const std::string &Str) noexcept {
  char *Buf = MemInst.getPointer<char *>(Offset);
  std::copy_n(Str.c_str(), Str.length(), Buf);
}
} // namespace

TEST(WasmEdgeProcessTest, SetProgName) {
  // Create the wasmedge_process module instance.
  auto *ProcMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessModule *>(createModule());
  EXPECT_FALSE(ProcMod == nullptr);

  // Create the memory instance.
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  // Clear the memory[0, 64].
  fillMemContent(MemInst, 0, 64);
  // Set the memory[0, 4] as string "echo".
  fillMemContent(MemInst, 0, std::string("echo"));

  // Get the function "wasmedge_process_set_prog_name".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_set_prog_name");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessSetProgName &>(
          FuncInst->getHostFunc());

  // Test: Run function successfully.
  EXPECT_TRUE(HostFuncInst.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().Name, "echo");

  // Test: Run function with nullptr memory instance -- fail
  EXPECT_FALSE(HostFuncInst.run(
      nullptr,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));

  delete ProcMod;
}

TEST(WasmEdgeProcessTest, AddArg) {
  // Create the wasmedge_process module instance.
  auto *ProcMod =
      dynamic_cast<WasmEdge::Host::WasmEdgeProcessModule *>(createModule());
  EXPECT_FALSE(ProcMod == nullptr);

  // Create the memory instance.
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  // Clear the memory[0, 64].
  fillMemContent(MemInst, 0, 64);
  // Set the memory[0, 4] as string "echo".
  fillMemContent(MemInst, 0, std::string("arg1"));
  // Set the memory[4, 8] as string "arg2".
  fillMemContent(MemInst, 4, std::string("arg2"));
  // Set the memory[30, 41] as string "--final-arg".
  fillMemContent(MemInst, 30, std::string("--final-arg"));

  // Get the function "wasmedge_process_add_arg".
  auto *FuncInst = ProcMod->findFuncExports("wasmedge_process_add_arg");
  EXPECT_NE(FuncInst, nullptr);
  EXPECT_TRUE(FuncInst->isHostFunction());
  auto &HostFuncInst = dynamic_cast<WasmEdge::Host::WasmEdgeProcessAddArg &>(
      FuncInst->getHostFunc());

  // Test: Run function successfully to add "arg1".
  EXPECT_TRUE(HostFuncInst.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().Args.size(), 1U);
  EXPECT_EQ(ProcMod->getEnv().Args[0], "arg1");

  // Test: Run function successfully to add "arg2".
  EXPECT_TRUE(HostFuncInst.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(4), UINT32_C(4)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().Args.size(), 2U);
  EXPECT_EQ(ProcMod->getEnv().Args[1], "arg2");

  // Test: Run function successfully to add "--final-arg".
  EXPECT_TRUE(HostFuncInst.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(30), UINT32_C(11)},
      {}));
  EXPECT_EQ(ProcMod->getEnv().Args.size(), 3U);
  EXPECT_EQ(ProcMod->getEnv().Args[2], "--final-arg");

  // Test: Run function with nullptr memory instance -- fail
  EXPECT_FALSE(HostFuncInst.run(
      nullptr,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));

  delete ProcMod;
}

/*
TEST(WasmEdgeProcessTest, AddEnv) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));
  WasmEdge::Host::WasmEdgeProcessAddEnv WasmEdgeProcessAddEnv(Env);
  fillMemContent(MemInst, 0, 256);
  char *Env1 = MemInst.getPointer<char *>(0);
  std::copy_n(std::string("ENV1").c_str(), 4, Env1);
  char *Val1 = MemInst.getPointer<char *>(4);
  std::copy_n(std::string("VALUE1").c_str(), 6, Val1);
  char *Env2 = MemInst.getPointer<char *>(30);
  std::copy_n(std::string("LD_LIBRARY_PATH").c_str(), 15, Env2);
  char *Val2 = MemInst.getPointer<char *>(50);
  std::copy_n(std::string("/usr/local/lib").c_str(), 14, Val2);

  EXPECT_TRUE(WasmEdgeProcessAddEnv.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4),
                                                  UINT32_C(4), UINT32_C(6)},
      {}));
  EXPECT_EQ(Env.Envs.size(), 1U);
  EXPECT_EQ(Env.Envs["ENV1"], "VALUE1");
  EXPECT_TRUE(WasmEdgeProcessAddEnv.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(30), UINT32_C(15),
                                                  UINT32_C(50), UINT32_C(14)},
      {}));
  EXPECT_EQ(Env.Envs.size(), 2U);
  EXPECT_EQ(Env.Envs["LD_LIBRARY_PATH"], "/usr/local/lib");
  EXPECT_FALSE(WasmEdgeProcessAddEnv.run(
      nullptr,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4),
                                                  UINT32_C(4), UINT32_C(6)},
      {}));
}

TEST(WasmEdgeProcessTest, AddStdIn) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));
  WasmEdge::Host::WasmEdgeProcessAddStdIn WasmEdgeProcessAddStdIn(Env);
  fillMemContent(MemInst, 0, 64);
  uint8_t *Buf1 = MemInst.getPointer<uint8_t *>(0);
  std::copy_n(std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04}).begin(), 4, Buf1);
  uint8_t *Buf2 = MemInst.getPointer<uint8_t *>(30);
  std::copy_n(std::vector<uint8_t>{'h', 'e', 'l', 'l', 'o', ' ', 's', 's', 'v',
                                   'm', '\n'}
                  .begin(),
              11, Buf2);

  EXPECT_TRUE(WasmEdgeProcessAddStdIn.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));
  EXPECT_EQ(Env.StdIn.size(), 4U);
  EXPECT_EQ(Env.StdIn, std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04}));
  EXPECT_TRUE(WasmEdgeProcessAddStdIn.run(
      &MemInst,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(30), UINT32_C(11)},
      {}));
  EXPECT_EQ(Env.StdIn.size(), 15U);
  EXPECT_EQ(Env.StdIn,
            std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04, 'h', 'e', 'l', 'l',
                                  'o', ' ', 's', 's', 'v', 'm', '\n'}));
  EXPECT_FALSE(WasmEdgeProcessAddStdIn.run(
      nullptr,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      {}));
}

TEST(WasmEdgeProcessTest, SetTimeOut) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Host::WasmEdgeProcessSetTimeOut WasmEdgeProcessSetTimeOut(Env);

  EXPECT_TRUE(WasmEdgeProcessSetTimeOut.run(
      nullptr, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(100)}, {}));
  EXPECT_EQ(Env.TimeOut, 100U);
}

TEST(WasmEdgeProcessTest, Run) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Host::WasmEdgeProcessRun WasmEdgeProcessRun(Env);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  Env.Name = "c++";
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), -1);
  EXPECT_TRUE(Env.StdOut.size() == 0);
  EXPECT_TRUE(Env.StdErr.size() > 0);
  std::string ErrStr =
      "Permission denied: Command \"c++\" is not in the white list. Please use "
      "--allow-command=c++ or --allow-command-all to add \"c++\" command into "
      "the white list.\n";
  EXPECT_TRUE(std::equal(Env.StdErr.begin(), Env.StdErr.end(), ErrStr.begin()));

  Env.AllowedAll = true;
  Env.Name = "c++";
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 1);
  EXPECT_TRUE(Env.StdOut.size() == 0);
  EXPECT_TRUE(Env.StdErr.size() > 0);

  Env.AllowedAll = false;
  Env.AllowedCmd.insert("c++");
  Env.Name = "c++";
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 1);
  EXPECT_TRUE(Env.StdOut.size() == 0);
  EXPECT_TRUE(Env.StdErr.size() > 0);

  Env.AllowedAll = false;
  Env.AllowedCmd.clear();
  Env.AllowedCmd.insert("/bin/echo");
  Env.Name = "/bin/echo";
  Env.Args.push_back("123456 test");
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 0);
  EXPECT_TRUE(Env.StdOut.size() == 12);
  EXPECT_TRUE(Env.StdErr.size() == 0);
  std::string OutStr = "123456 test\n";
  EXPECT_TRUE(std::equal(Env.StdOut.begin(), Env.StdOut.end(), OutStr.begin()));
}

TEST(WasmEdgeProcessTest, GetExitCode) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Host::WasmEdgeProcessGetExitCode WasmEdgeProcessGetExitCode(Env);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  EXPECT_TRUE(WasmEdgeProcessGetExitCode.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<int32_t>(), 0);
}

TEST(WasmEdgeProcessTest, GetStdOut) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));
  WasmEdge::Host::WasmEdgeProcessRun WasmEdgeProcessRun(Env);
  WasmEdge::Host::WasmEdgeProcessGetStdOutLen WasmEdgeProcessGetStdOutLen(Env);
  WasmEdge::Host::WasmEdgeProcessGetStdOut WasmEdgeProcessGetStdOut(Env);
  fillMemContent(MemInst, 0, 256);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  Env.Name = "echo";
  Env.AllowedCmd.insert("echo");
  Env.Args.push_back("$(pwd)");
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_EQ(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_TRUE(WasmEdgeProcessGetStdOutLen.run(nullptr, {}, RetVal));
  uint32_t Len = RetVal[0].get<uint32_t>();
  EXPECT_TRUE(Len > 0U);
  EXPECT_FALSE(WasmEdgeProcessGetStdOut.run(
      nullptr, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)}, {}));
  EXPECT_TRUE(WasmEdgeProcessGetStdOut.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)}, {}));
  uint8_t *Buf = MemInst.getPointer<uint8_t *>(0);
  EXPECT_TRUE(std::equal(Env.StdOut.begin(), Env.StdOut.end(), Buf));
}

TEST(WasmEdgeProcessTest, GetStdErr) {
  WasmEdge::Host::WasmEdgeProcessEnvironment Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));
  WasmEdge::Host::WasmEdgeProcessRun WasmEdgeProcessRun(Env);
  WasmEdge::Host::WasmEdgeProcessGetStdErrLen WasmEdgeProcessGetStdErrLen(Env);
  WasmEdge::Host::WasmEdgeProcessGetStdErr WasmEdgeProcessGetStdErr(Env);
  fillMemContent(MemInst, 0, 256);
  std::array<WasmEdge::ValVariant, 1> RetVal;

  Env.Name = "c++";
  Env.AllowedCmd.insert("c++");
  EXPECT_TRUE(WasmEdgeProcessRun.run(nullptr, {}, RetVal));
  EXPECT_NE(RetVal[0].get<uint32_t>(), 0U);
  EXPECT_TRUE(WasmEdgeProcessGetStdErrLen.run(nullptr, {}, RetVal));
  uint32_t Len = RetVal[0].get<uint32_t>();
  EXPECT_TRUE(Len > 0);
  EXPECT_FALSE(WasmEdgeProcessGetStdErr.run(
      nullptr, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)}, {}));
  EXPECT_TRUE(WasmEdgeProcessGetStdErr.run(
      &MemInst, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)}, {}));
  uint8_t *Buf = MemInst.getPointer<uint8_t *>(0);
  EXPECT_TRUE(std::equal(Env.StdErr.begin(), Env.StdErr.end(), Buf));
}

TEST(WasmEdgeProcessTest, Module) {
  WasmEdge::Host::WasmEdgeProcessModule Mod =
      WasmEdge::Host::WasmEdgeProcessModule();
  EXPECT_EQ(Mod.getEnv().ExitCode, 0U);
  EXPECT_EQ(Mod.getFuncExportNum(), 11U);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_set_prog_name"), nullptr);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_add_arg"), nullptr);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_add_env"), nullptr);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_add_stdin"), nullptr);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_set_timeout"), nullptr);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_run"), nullptr);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_get_exit_code"), nullptr);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_get_stdout_len"), nullptr);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_get_stdout"), nullptr);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_get_stderr_len"), nullptr);
  EXPECT_NE(Mod.findFuncExports("wasmedge_process_get_stderr"), nullptr);
}
*/

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
