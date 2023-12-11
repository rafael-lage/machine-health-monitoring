#include <iostream>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <thread>
#include <unistd.h>
#include "json.hpp" // Manipulação de JSON
#include "mqtt/client.h" // Paho MQTT
#include <iomanip>
#include <mutex>

#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"

// Variáveis globais
mqtt::client client(BROKER_ADDRESS, "sensor-monitor");
std::string machineId;
std::mutex mtx;

// Protótipo
void create_main_msg(nlohmann::json& j, std::string sensor_id, std::string data_type, int frequency, std::string sensor_id2, std::string data_type2, int frequency2);
void create_sensor_msg(nlohmann::json& j, std::string sensor_id, int value);
void publish_main_topic(nlohmann::json& j);
void publish_sensor_topic(nlohmann::json& j, int frequency);

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Erro - Favor informar a frequência do sensor no formato:"
                  << std::endl << "sensor_monitor <machine_frequency> <temperature_sensor_frequency> <memory_sensor_frequency>"
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
    int machine_frequency = atoi(argv[1]);
    int temperature_sensor_frequency = atoi(argv[2]);
    int memory_sensor_frequency = atoi(argv[3]);

    try {
        client.connect(connOpts);
    } catch (mqtt::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::clog << "Connected to the broker" << std::endl;

    // Iniciar uma thread para a mensagem principal
    std::thread main_msg_thread([machine_frequency, temperature_sensor_frequency, memory_sensor_frequency]() {
        while (true) {
            nlohmann::json main_j;
            create_main_msg(main_j, "cpu_temperature", "percentage", temperature_sensor_frequency, "memory_usage", "percentage", memory_sensor_frequency);
            publish_main_topic(main_j);

            // Dormir por algum tempo antes da próxima iteração
            std::this_thread::sleep_for(std::chrono::milliseconds(machine_frequency));
        }
    });

    // Iniciar uma thread para o sensor de temperatura
    std::thread sensor_thread([temperature_sensor_frequency]() {
        while (true) {
            nlohmann::json sensor_j;
            int sensor_value = rand() % 101;
            create_sensor_msg(sensor_j, "cpu_temperature", sensor_value);
            publish_sensor_topic(sensor_j, temperature_sensor_frequency);

            // Dormir por algum tempo antes da próxima iteração
            std::this_thread::sleep_for(std::chrono::milliseconds(temperature_sensor_frequency));
        }
    });

    // Iniciar uma thread para o sensor de memoria
    std::thread sensor_thread2([memory_sensor_frequency]() {
        while (true) {
            nlohmann::json sensor_j2;
            int sensor_value = rand() % 101;
            create_sensor_msg(sensor_j2, "memory_usage", sensor_value);
            publish_sensor_topic(sensor_j2, memory_sensor_frequency);

            // Dormir por algum tempo antes da próxima iteração
            std::this_thread::sleep_for(std::chrono::milliseconds(memory_sensor_frequency));
        }
    });

    // Aguardar que as threads terminem (isso nunca deve acontecer neste caso)
    main_msg_thread.join();
    sensor_thread.join();
    sensor_thread2.join();
    
    // Fechar a conexão MQTT antes de sair.
    try {
        client.disconnect();
    } catch (const mqtt::exception& exc) {
        std::cerr << "Error during disconnect: " << exc.what() << std::endl;
    }

    return EXIT_SUCCESS;
}

// Definição da função create_main_msg.
void create_main_msg(nlohmann::json& j, std::string sensor_id, std::string data_type, int frequency, std::string sensor_id2, std::string data_type2, int frequency2) {
    // Construir a mensagem JSON para a main_j.
    j["sensor_id"] = sensor_id;
    j["data_type"] = data_type;
    j["data_interval"] = frequency;
    j["sensor_id2"] = sensor_id2;
    j["data_type2"] = data_type2;
    j["data_interval2"] = frequency2;
}

// Definição da função create_sensor_msg.
void create_sensor_msg(nlohmann::json& j, std::string sensor_id, int value) {
    // Construir a mensagem JSON para sensor_j.
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_c);
    std::stringstream ss;
    ss << std::put_time(now_tm, "%FT%TZ");
    std::string timestamp = ss.str();

    j["timestamp"] = timestamp;
    j["sensor_id"] = sensor_id;
    j["value"] = value;
}

// Função para publicar a mensagem principal
void publish_main_topic(nlohmann::json& j) {
    mtx.lock();
    // Publicar a mensagem JSON no tópico apropriado.
    std::string topic = "/sensors/" + machineId +
                        "/" + j["sensor_id"].get<std::string>() +
                        "/" + j["data_type"].get<std::string>() +
                        "/" + std::to_string(j["data_interval"].get<int>()) +
                        "/" + j["sensor_id2"].get<std::string>() +
                        "/" + j["data_type2"].get<std::string>() +
                        "/" + std::to_string(j["data_interval2"].get<int>());
    mqtt::message msg(topic, j.dump(), QOS, false);

    std::clog << "Main Message published - topic: " << topic << std::endl;

    client.publish(msg);
    mtx.unlock();
}

// Função para publicar a mensagem do sensor
void publish_sensor_topic(nlohmann::json& j, int frequency) {
    mtx.lock();
    // Publicar a mensagem JSON no tópico apropriado.
    std::string topic = "/sensors/" + machineId +
                        "/" + j["sensor_id"].get<std::string>() +
                        "/" + j["timestamp"].get<std::string>() +
                        "/" + std::to_string(j["value"].get<int>());
    mqtt::message msg(topic, j.dump(), QOS, false);

    std::clog << "Sensor Message published - topic: " << topic << std::endl;

    client.publish(msg);
    mtx.unlock();
}
