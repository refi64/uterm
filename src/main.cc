#include "uterm.h"

#include <absl/debugging/symbolize.h>
#include <absl/debugging/failure_signal_handler.h>

int main(int argc, char **argv) {
  absl::InitializeSymbolizer(argv[0]);
  absl::InstallFailureSignalHandler({});

  return gUterm.Run();
}
