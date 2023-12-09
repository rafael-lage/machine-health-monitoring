//controlar a leitura de dados para utilizar na logica
//implementar a comunicacao com a interface grafica
//implementar as funcoes solicitadas pelo professor
//expandir para dois sensores

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <unistd.h>
#include "json.hpp" 
#include "mqtt/client.h" 

#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"
#define GRAPHITE_HOST "graphite"
#define GRAPHITE_PORT 2003

const std::string SERVER_ADDRESS("tcp://localhost:1883");
const std::string CLIENT_ID("ExampleSubscriber");
const std::string TOPIC("/sensors/#");

//executa apos receber os dados coletados do broker
void post_metric(const std::string& machine_id, const std::string& sensor_id, const std::string& timestamp_str, const int value) {
    std :: cout << "Mensagem recebida: "<< machine_id << " " << sensor_id << " " << timestamp_str << " " << value << std:: endl;
}

std::vector<std::string> split(const std::string &str, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

    // Create an MQTT callback.
    class callback : public virtual mqtt::callback {
    public:

        void message_arrived(mqtt::const_message_ptr msg) override {
            auto j = nlohmann::json::parse(msg->get_payload());

            std::string topic = msg->get_topic();
            auto topic_parts = split(topic, '/');
            std::string machine_id = topic_parts[2];
            std::string sensor_id = topic_parts[3];

            std::string timestamp = j["timestamp"];
            int value = j["value"];
            post_metric(machine_id, sensor_id, timestamp, value);
        }
    };

int main(int argc, char* argv[]) {
    mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);
    callback cb;
    client.set_callback(cb);

    // Conectando ao broker
    mqtt::connect_options connOpts;
    mqtt::token_ptr conntok = client.connect(connOpts);
    conntok->wait();

    // Assinando ao tópico
    client.subscribe(TOPIC, 1);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }


    // Desconectando
    client.disconnect()->wait();

    return 0;
}
