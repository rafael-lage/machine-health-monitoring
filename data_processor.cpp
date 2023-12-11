#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <unistd.h>
#include "json.hpp"
#include "mqtt/client.h"
#include <vector>

#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"
#define GRAPHITE_HOST "graphite"
#define GRAPHITE_PORT 2003

const std::string SERVER_ADDRESS("tcp://localhost:1883");
const std::string CLIENT_ID("ExampleSubscriber");
const std::string TOPIC("/sensors/#");

struct Main_Message {
    std::string machine_id = " ";
    std::string sensor_id = " ";
    std::string sensor_id2 = " ";
    std::string data_type = " ";
    std::string data_type2 = " ";
    int frequency = 0;
    int frequency2 = 0;
};

struct Sensor_Message {
    std::string machine_id;
    std::string sensor_id;
    std::string timestamp;
    int value;
};

struct Processed_Data {
    std::string machine_id;
    std::string sensor_id;
    std::string timestamp;
    int current_value = 0;
    int old_value = 0;
    int number_of_values = 1;
    int sum_of_values = 0;
    int avg_value = 0;
    int max_value = 0;
    int min_value = 100;
};

int InactivityDetected = 0;

check_inactivity();


void create_main_msg(Main_Message& msg, const std::string& machine_id, const std::string& sensor_id, const std::string& data_type, const int frequency, const std::string& sensor_id2, const std::string& data_type2, const int frequency2) {
    msg.machine_id = machine_id;
    msg.sensor_id = sensor_id;
    msg.sensor_id2 = sensor_id2;
    msg.data_type = data_type;
    msg.data_type2 = data_type2;
    msg.frequency = frequency;
    msg.frequency2 = frequency2;
}

void create_sensor_msg(Sensor_Message& msg, const std::string& machine_id, const std::string& sensor_id, const std::string& timestamp_str, const int value) {
    msg.machine_id = machine_id;
    msg.sensor_id = sensor_id;
    msg.timestamp = timestamp_str;
    msg.value = value;
}

void post_metric_main(const Main_Message& main_msg) {
    std::cout << "Mensagem Main recebida: " << main_msg.machine_id << " "
              << main_msg.sensor_id << " "
              << main_msg.data_type << " "
              << main_msg.frequency << " "
              << main_msg.sensor_id2 << " "
              << main_msg.data_type2 << " "
              << main_msg.frequency2
              << std::endl;
}

void create_processed_data(Processed_Data& data, std::string machine_id, std::string sensor_id, std::string timestamp, int value, int last_value_temperature, int n) {

    //recebe os parametros
    data.machine_id = machine_id;
    data.sensor_id = sensor_id;
    data.timestamp = timestamp;
    data.current_value = value;
    data.old_value = last_value_temperature;
    data.number_of_values = n;

    //pega os menores e maiores valores que aparecerem
    if (data.current_value > data.max_value) {
        data.max_value = data.current_value;
    }

    if (data.current_value < data.min_value) {
        data.min_value = data.current_value;
    }

    //pega o acumulado de valores
    data.sum_of_values = data.sum_of_values + data.current_value;

    //pega a media dos valores
    data.avg_value = data.sum_of_values / data.number_of_values;
}

int last_value_temperature = 0;
int temperature_n = 1;

int last_value_memory = 0;
int memory_n = 1;

Processed_Data data_temperature;
Processed_Data data_memory;

void post_metric_sensor(const std::string& machine_id, const std::string& sensor_id, const std::string& timestamp_str, const int value) {
    Sensor_Message msg; //inicializa o objeto com as informacoes recebidas do sensor
    create_sensor_msg(msg, machine_id, sensor_id, timestamp_str, value); //cria o objeto com os parametros recebidos
    std::cout << "Mensagem do Sensor recebida: " << msg.machine_id << " " << msg.sensor_id << " " << msg.timestamp << " " << msg.value << std::endl;

    if (msg.sensor_id == "cpu_temperature") {
        //gera os dados que serao enviados para o graphit
        create_processed_data(data_temperature, msg.machine_id, msg.sensor_id, msg.timestamp, msg.value, last_value_temperature, temperature_n);

        std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;
        std::cout << "Valor atual: " << data_temperature.current_value << std::endl;
        std::cout << "Valor minimo: " << data_temperature.min_value << std::endl;
        std::cout << "Valor maximo: " << data_temperature.max_value << std::endl;
        std::cout << "Somatorio: " << data_temperature.sum_of_values << std::endl;
        std::cout << "Numero de dados: " << data_temperature.number_of_values << std::endl;
        std::cout << "Media: " << data_temperature.avg_value << std::endl;
        std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;

        last_value_temperature = msg.value;
        temperature_n++;
    }

    if (msg.sensor_id == "memory_usage") {
        //gera os dados que serao enviados para o graphit
        create_processed_data(data_memory, msg.machine_id, msg.sensor_id, msg.timestamp, msg.value, last_value_memory, memory_n);

        std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;
        std::cout << "Valor atual: " << data_memory.current_value << std::endl;
        std::cout << "Valor minimo: " << data_memory.min_value << std::endl;
        std::cout << "Valor maximo: " << data_memory.max_value << std::endl;
        std::cout << "Somatorio: " << data_memory.sum_of_values << std::endl;
        std::cout << "Numero de dados: " << data_memory.number_of_values << std::endl;
        std::cout << "Media: " << data_memory.avg_value << std::endl;
        std::cout << "--------------------------------------------------------------------------------------------------------" << std::endl;

        last_value_memory = msg.value;
        memory_n++;
    }
}

std::vector<std::string> split(const std::string& str, char delim) {
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

        //std::cout << "Topic[4] = "<< topic_parts[4] << std::endl;
        //std::cout << "Topic[4].size = "<< topic_parts[4].size() << std::endl;
        std::cout << "Topic.size = " << topic_parts.size() << std::endl;

        //for (int i = 0; i<2 ;i++)
        //    std:: cout << "Topic["<< i << "]" << topic_parts[i] << std::endl;
        //verifica se a mensagem vem do sensor pelo tamanho do paramatro 4. Se for 20 provavelmente recebeu o timestamp
        if (topic_parts[4].size() == 20) {
            std::string timestamp = topic_parts[4];
            int value = std::stoi(topic_parts[5]);

            post_metric_sensor(machine_id, sensor_id, timestamp, value);

            //verifica se o numero de parametros condiz com o que se espera receber pela quantidade de sensores
            // formato: /sensors/1cf7ac207520/cpu_temperature/int/1000/memory_usage/int/2000
        } else if (topic_parts.size() == 9) {

            std::string data_type = topic_parts[4];
            int frequency = std::stoi(topic_parts[5]);
            std::string sensor_id2 = topic_parts[6];
            std::string data_type2 = topic_parts[7];
            int frequency2 = std::stoi(topic_parts[8]);

            Main_Message main_msg;
            create_main_msg(main_msg, machine_id, sensor_id, data_type, frequency, sensor_id2, data_type2, frequency2);

            post_metric_main(main_msg);

        } else {
            std::cerr << "Formato de tópico não reconhecido: " << topic << std::endl;
        }
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
