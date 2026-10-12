set_default(false)
set_group("test")

-- Provides handle_contract_violation for the test executables; see support/contract_handler.cpp.
target("lilac-test-support")
    set_kind("object")
    add_files("support/contract_handler.cpp")

includes("*/xmake.lua")
