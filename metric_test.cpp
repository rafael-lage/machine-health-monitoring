#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#define GRAPHITE_HOST "graphite"
#define GRAPHITE_HOST "127.0.0.1"
#define GRAPHITE_PORT 2003

int sendMetricToGraphite(const std::string& metric, int value, std::string& timestamp) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(GRAPHITE_PORT);
    
    // Convert host name to IP address
    if (inet_pton(AF_INET, GRAPHITE_HOST, &server_addr.sin_addr) <= 0) {
        std::cerr << "Error converting host to IP address" << std::endl;
        close(sockfd);
        return 1;
    }

    if (connect(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        std::cerr << "Error connecting to Graphite: " << strerror(errno) << std::endl;
        close(sockfd);
        return 1;
    }

    // Construct the message
    std::ostringstream oss;
    oss << metric << " " << value << " " << timestamp << "\n";
    std::string message = oss.str();

    std::cout << "Sending message to Graphite: " << message;

    ssize_t sent = send(sockfd, message.c_str(), message.size(), 0);
    if (sent < 0) {
        std::cerr << "Error sending metric to Graphite: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Metric sent successfully to Graphite" << std::endl;
    }

    close(sockfd);
    return 0;
}



int main() {
    std::string test_metric = "myapp.my_metric";
    int test_value = 42;

    // Get the current timestamp (you may use a library like Boost or C++20's chrono for a more robust solution)
    time_t now = time(nullptr);
    std::string timestamp = std::to_string(now);

    return sendMetricToGraphite(test_metric, test_value, timestamp);
}
