// esta publicando corretamente
// falta ajustar formatacao das mensagens principal e dos sensores
// precisa consertar para separar a threads de mensagem principal e dos sensores
// threads dos sensores estao esperando a thread principal
// expandir para dois sensores

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <thread>
#include <unistd.h>
#include "json.hpp" // Manipulação de JSON
#include "mqtt/client.h" // Paho MQTT
#include <iomanip>

#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"

// Variáveis globais
mqtt::client client(BROKER_ADDRESS, "sensor-monitor");
std::string machineId;

// Protótipo
void create_main_msg(nlohmann::json& j, std::string data_type, std::string sensor_id, int frequency);
void create_sensor_msg(nlohmann::json& j, std::string sensor_id, std::string timestamp, int value);
void publish_topic(nlohmann::json& j, int frequency);


int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Erro - Favor informar a frequência do sensor no formato:"
        << std::endl << "sensor_monitor <temperature_frequency> <temperature_sensor_frequency>"
        << std::endl;
        return EXIT_FAILURE;
    }

    // Obtendo o identificador único da máquina, neste caso, o nome do host.
    char hostname[1024];
    gethostname(hostname, 1024);
    machineId = std::string(hostname);

    // Connect to the MQTT broker.
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);

    // Recebendo a frequência do primeiro sensor.
    int temperature_frequency = atoi(argv[1]);
    int temperature_sensor_frequency = atoi(argv[2]);

    try {
        client.connect(connOpts);
    } catch (mqtt::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::clog << "Connected to the broker" << std::endl;

    while (true) {
        // Get the current time in ISO 8601 format.
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);
        std::stringstream ss;
        ss << std::put_time(now_tm, "%FT%TZ");
        std::string timestamp = ss.str();

        // Construir a mensagem JSON para cpu_temperature.
        nlohmann::json j;
        nlohmann::json main_j;

        int sensor_value = rand() % 101;
        create_main_msg(main_j, "int", "cpu_temperature",temperature_frequency);
        create_sensor_msg(j,"cpu_temperature",timestamp,sensor_value);
        
        publish_topic(main_j,temperature_frequency);
        publish_topic(j,temperature_sensor_frequency);

    }

    return EXIT_SUCCESS;
}

// Definição da função create_main_msg.
void create_main_msg(nlohmann::json& j, std::string data_type, std::string sensor_id, int frequency) {
    // Construir a mensagem JSON para maquina.
    j["data_type"] = data_type;
    j["data_interval"] = frequency;
    //j["timestamp"] = timestamp;
    j["sensor_id"] = sensor_id;
    //j["value"] = rand() % 101;

}

// Definição da função create_sensor_msg.
void create_sensor_msg(nlohmann::json& j, std::string sensor_id, std::string timestamp, int value) {
    // Construir a mensagem JSON para maquina.
    j["timestamp"] = timestamp;
    j["sensor_id"] = sensor_id;
    j["value"] = value;
}

void publish_topic(nlohmann::json& j, int frequency){
        // Publicar a mensagem JSON no tópico apropriado.
        std::string topic = "/sensors/" + machineId + "/" + j["sensor_id"].get<std::string>();
        mqtt::message msg(topic, j.dump(), QOS, false);
        std::clog << "Message published - topic: " << topic << " - Message: " << j.dump() << std::endl;
        client.publish(msg);
        // Dormir por algum tempo.
        std::this_thread::sleep_for(std::chrono::milliseconds(frequency));
}

