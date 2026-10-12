add_requires("ngcpp-proxy")

target("lilac")
    set_kind("static")
    add_files("*.cpp", "language/*.cpp")
    -- `xmake install` (and the xmake package) lays headers out as include/lilac/...
    add_headerfiles("(**.h)|tests/**.h", {prefixdir = "lilac"})
    add_syslinks("stdc++exp", {public = true})
    -- lexer.h exposes pro::proxy, so consumers need its headers too
    add_packages("ngcpp-proxy", {public = true})

includes("tests/xmake.lua")
