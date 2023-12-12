//FAZER UM CÓDIGO TESTE QUE ENVIA UMA METRICA SIMPLES UTILIZANDO TCP-IP
//EXPANDIR A LOGICA PARA ENVIAR TODAS AS METRICAS QUE VAO SER NECESSARIAS

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <unistd.h>
#include "json.hpp"
#include "mqtt/client.h"
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define QOS 1
#define BROKER_ADDRESS "tcp://localhost:1883"
#define GRAPHITE_HOST "127.0.0.1"
//#define GRAPHITE_HOST "graphite"
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

std::time_t convertTimestampToEpoch(const std::string& timestamp) {
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail()) {
        // Handle parsing error
        return -1; // Indicate error
    }
    auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::seconds>(time_point.time_since_epoch()).count();
}

 void send_processed_data(Processed_Data& data){

            std::string identifier = data.machine_id + "_" + data.sensor_id;
            std::string buffer = identifier;
            

            std::time_t epoch_time = convertTimestampToEpoch(data.timestamp);
            if (epoch_time == -1) {
                // Handle error in timestamp conversion
                std::cerr << "Error converting timestamp to epoch time." << std::endl;
                return;
            }
            std::string timestamp = std::to_string(epoch_time);



            buffer = identifier + ".value";
            sendMetricToGraphite(buffer, data.current_value, timestamp);
            buffer = identifier + ".avg";
            sendMetricToGraphite(buffer, data.avg_value, timestamp);
            buffer = identifier + ".max";
            sendMetricToGraphite(buffer, data.max_value, timestamp);
            buffer = identifier + ".min";
            sendMetricToGraphite(buffer, data.min_value, timestamp);
            buffer = identifier + ".number_of_values";
            sendMetricToGraphite(buffer, data.number_of_values, timestamp);

 }


//global variables to detect inactivity

bool InactivityDetected = false;
int countSensorMsg = 0;
int countSensor2Msg = 0;

//prototypes
void startInactivitySystem(Main_Message msg);
void countInactivity (int max_time, int max_time_2);

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
        //reseta contador de inatividade
        countSensorMsg = 0;
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

         send_processed_data(data_temperature);
    }

    if (msg.sensor_id == "memory_usage") {
        //reseta contador de inatividade
        countSensor2Msg = 0;
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
        
        send_processed_data(data_memory);
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

void startInactivitySystem(Main_Message msg){

    //criando limite para ser considerado timeout em segundos
    int max_time = (msg.frequency * 10)/1000;
    int max_time2 = (msg.frequency2 * 10)/1000;

    //criando uma thread que vai contando o tempo desde a ultima mensagem
    std::thread ControlThread (countInactivity, max_time, max_time2);

    ControlThread.detach();
}

void countInactivity (int max_time, int max_time2){
    
    while(1){
        std::this_thread::sleep_for(std::chrono::seconds(1));
       // mtx.lock();
        countSensorMsg += 1;
        countSensor2Msg += 1;

        //std::cout << "max_1 = " << max_time <<std::endl << "max_2 = " << max_time2 <<std::endl;
        //std::cout << "Count1 = " << countSensorMsg <<std::endl << "Count2 = " << countSensor2Msg <<std::endl;

        if(countSensorMsg == max_time || countSensor2Msg == max_time2){
            InactivityDetected = true;
            std::cout << "Inatividade Detectada" << std::endl;
        }

        //mtx.unlock();
    }

}


bool isFirstExecution = true;
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
        //std::cout << "Topic.size = " << topic_parts.size() << std::endl;

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

            if(isFirstExecution){
                startInactivitySystem(main_msg);
                isFirstExecution = false;
            }
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
