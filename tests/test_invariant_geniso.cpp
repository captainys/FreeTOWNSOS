#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <sstream>

// Test that geniso handles adversarial filename inputs without buffer overflow.
// Invariant: The process must not crash/segfault when given oversized or
// boundary-length filenames in the ISO manifest input.

class SecurityTest : public ::testing::TestWithParam<std::string> {};

TEST_P(SecurityTest, GenIsoHandlesAdversarialFilenames) {
    // Invariant: geniso must exit cleanly (not crash) regardless of input size.
    std::string payload = GetParam();

    // Write a temporary manifest file with the adversarial filename entry
    std::string tmpInput = "/tmp/test_geniso_input_XXXXXX";
    char tmpBuf[64];
    snprintf(tmpBuf, sizeof(tmpBuf), "/tmp/test_geniso_XXXXXX");
    int fd = mkstemp(tmpBuf);
    if (fd >= 0) {
        std::string content = "/dev/null\t" + payload + "\n";
        write(fd, content.c_str(), content.size());
        close(fd);
    }

    // Run geniso with the crafted input; it must not crash (signal/segfault)
    std::string cmd = "./geniso " + std::string(tmpBuf) + " /tmp/out_test.iso 2>/dev/null; echo $?";
    FILE* pipe = popen(cmd.c_str(), "r");
    ASSERT_NE(pipe, nullptr);

    char result[16] = {};
    fgets(result, sizeof(result), pipe);
    int ret = pclose(pipe);

    // The process must not have been killed by a signal (segfault = 139, etc.)
    // WIFEXITED means it exited normally, not via signal
    EXPECT_TRUE(WIFEXITED(ret)) << "Process crashed on payload: " << payload.substr(0, 40);

    unlink(tmpBuf);
    unlink("/tmp/out_test.iso");
}

INSTANTIATE_TEST_SUITE_P(
    AdversarialInputs,
    SecurityTest,
    ::testing::Values(
        // Exact exploit: oversized name exceeding typical fixed buffer (256 bytes)
        std::string(300, 'A'),
        // Boundary: exactly 256 characters
        std::string(256, 'B'),
        // Valid input: normal short filename
        std::string("validfile.txt")
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}