// macOS: the import worker sandbox must confine writes to its transaction
// directory and deny network and exec, and a worker must exit with its
// parent. Each check runs in a forked child.
#include "legacy_mod_process.hpp"
#include <cassert>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace {
bool can_write(const std::filesystem::path& path) {
    std::ofstream out(path, std::ios::binary);
    return out && (out << "x") && out.flush();
}
} // namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() /
        ("dkr-mod-sandbox-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto inside = root / "transaction";
    std::filesystem::create_directories(inside);

    const pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        try {
            dkr::mods::sandbox_import_worker(inside);
        } catch (...) {
            _exit(10);
        }
        if (!can_write(inside / "staged.bin")) _exit(11);
        if (can_write(root / "escaped.bin")) _exit(12);
        const int network = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in loopback{};
        loopback.sin_family = AF_INET;
        loopback.sin_port = htons(9);
        loopback.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        // Seatbelt may refuse the socket or the connect; either way with EPERM.
        if (network < 0) {
            if (errno != EPERM) _exit(16);
        } else {
            if (connect(network, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback)) == 0) _exit(13);
            if (errno != EPERM) _exit(14);
        }
        char* arguments[] = {const_cast<char*>("/usr/bin/true"), nullptr};
        execv(arguments[0], arguments);
        _exit(errno == EPERM ? 0 : 15);
    }
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    const int code = WIFEXITED(status) ? WEXITSTATUS(status) : 100;
    std::filesystem::remove_all(root);
    if (code != 0) std::fprintf(stderr, "sandbox check failed: %d\n", code);
    assert(code == 0);
    std::puts("[test][mod-sandbox] writes confined, network and exec denied");

    // Parent-death watch: the intermediate parent exits while its constrained
    // worker sleeps; the worker must be gone within a few seconds.
    int pipe_fds[2]{};
    assert(pipe(pipe_fds) == 0);
    const pid_t intermediate = fork();
    assert(intermediate >= 0);
    if (intermediate == 0) {
        const pid_t worker = fork();
        if (worker == 0) {
            try { dkr::mods::constrain_import_worker(); } catch (...) { _exit(10); }
            for (;;) pause();
        }
        if (worker < 0 || write(pipe_fds[1], &worker, sizeof(worker)) != sizeof(worker)) _exit(11);
        _exit(0);
    }
    pid_t worker = 0;
    assert(read(pipe_fds[0], &worker, sizeof(worker)) == sizeof(worker));
    assert(waitpid(intermediate, &status, 0) == intermediate && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    bool exited = false;
    for (int attempt = 0; attempt < 200 && !exited; ++attempt) {
        exited = kill(worker, 0) != 0 && errno == ESRCH;
        if (!exited) std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    if (!exited) { kill(worker, SIGKILL); std::fprintf(stderr, "worker outlived its parent\n"); }
    assert(exited);
    std::puts("[test][mod-sandbox] worker exits with its parent");
    return 0;
}
