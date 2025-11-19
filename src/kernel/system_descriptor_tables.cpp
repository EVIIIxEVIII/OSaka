#include <optional>
#include "kernel/apic.hpp"

static std::optional<APICEntryIOAPIC>    io_apic;
static std::optional<APICEntryLocalAPIC> lapic;


