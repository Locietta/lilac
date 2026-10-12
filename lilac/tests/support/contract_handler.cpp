#include <contracts>

// On MinGW, libstdc++exp only provides a weak default for this handler, and a
// weak definition in a static archive does not resolve the references, so
// every executable that links lilac has to define it. lilac itself does not,
// because the application (or another library such as lighter) owns it.
void handle_contract_violation(const std::contracts::contract_violation &violation) noexcept {
    std::contracts::invoke_default_contract_violation_handler(violation);
}
