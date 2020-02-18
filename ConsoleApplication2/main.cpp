#include <iostream>
#include <WS2tcpip.h>
#include <fstream>
#include <vector>
#include <string>
#include <numeric>
#include <chrono>
#include <ctime>

#pragma comment (lib, "ws2_32.lib") //Including library


constexpr auto BUFFER_LEN = 10;
constexpr auto NR_OF_DATATYPES = 16;
constexpr auto NR_OF_READINGS_FOR_AVE = 10;

using namespace std;

//Adds the last value from sensor to the vector. (vector contains the last 10 readings)
void add_to_last_10(vector<uint64_t> &sens_vector, uint64_t value) {		
	if (sens_vector.size() < NR_OF_READINGS_FOR_AVE) {							//If there aren't yet 10 values in vector, i just adds the last member to the front
		sens_vector.insert(sens_vector.begin(), value);
		return;
	}
	else {																		//If there are already 10 members in vector, then one is added to the front and the last one is removed
		sens_vector.insert(sens_vector.begin(), value);
		sens_vector.pop_back();
	}

}

//Saves sensor information to "Sensor data.txt" in csv format
void save_to_txt(int sens_nr, int sens_type, uint64_t value) {				
	ofstream myfile("Sensor data.txt", ios_base::app);
	myfile << sens_nr << ",";
	myfile << sens_type << ",";
	myfile << value << "\n";
	myfile.close();
}

//Calculates and returns the average value of the vector
uint64_t get_v_ave(vector<uint64_t> sens_vector) {
	uint64_t average = 0;
	if (sens_vector.size() != 0) {
		for (unsigned int i = 0; i < sens_vector.size(); i++) {
			average += sens_vector[i];
		}
		return average / sens_vector.size();
	}
	else {
		return 0;
	}
}

/*
uint64_t get_ave(int sensor) {
	uint64_t average;
	
	string line;
	ifstream file;

	// Open an existing file 
	file.open("Sensor data.txt", ios::in);

	




	return average;
}
*/

int main()
{

	char				buf[BUFFER_LEN];		//Buffer for storing incoming sensor data				
	char				reqbuf[2];				//Buffer for storing incoming request data
	uint64_t			ui_sens_64 = 0;			//Int for storing the value of the sensor
	chrono::time_point<chrono::system_clock>	start, end;			//Timepoints for outputting at 1 Hz
	std::chrono::duration<double>				elapsed_seconds;

	//Vectors for storing the last 10 values of the sensors
	struct sensor {
		vector<uint64_t> vector;	//Values
		int type = 0;				//Value type
	};
	sensor sensor_0, sensor_1, sensor_2, sensor_3, sensor_4, sensor_5, sensor_6, sensor_7, sensor_8, sensor_9;

	//Starting up Winsock2
	WSADATA data;
	WORD version = MAKEWORD(2, 2);
	int wsOk = WSAStartup(version, &data);
	if (wsOk != 0)
	{
		std::cout << "Can't start Winsock! " << wsOk;
		return 1;
	}

	//Creating UDP sockets
	SOCKET sensors_in = socket(AF_INET, SOCK_DGRAM, 0);
	/*
	SOCKET request_in = socket(AF_INET, SOCK_DGRAM, 0);

	//Setting up server address and port
	sockaddr_in request_in_hint;
	request_in_hint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	request_in_hint.sin_family = AF_INET; // Address format is IPv4
	request_in_hint.sin_port = htons(12346); // Convert from little to big endian

	//Binding it to socket
	if (bind(request_in, (sockaddr*)&request_in_hint, sizeof(request_in_hint)) == SOCKET_ERROR)
	{
		std::cout << "Can't bind socket! " << WSAGetLastError() << endl;
		return 1;
	}
	*/

	sockaddr_in sensors_in_hint;
	sensors_in_hint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	sensors_in_hint.sin_family = AF_INET; // Address format is IPv4
	sensors_in_hint.sin_port = htons(12345); // Convert from little to big endian

	if (bind(sensors_in, (sockaddr*)&sensors_in_hint, sizeof(sensors_in_hint)) == SOCKET_ERROR)
	{
		std::cout << "Can't bind socket! " << WSAGetLastError() << endl;
		return 1;
	}

	//Setting up info about client
	sockaddr_in client; 
	int clientLength = sizeof(client); // The size of the client information

	//Starting the timer
	start = chrono::system_clock::now();

	while (true)
	{
		//Clearing memory
		ZeroMemory(&client, clientLength);
		ZeroMemory(buf, BUFFER_LEN); 
		//ZeroMemory(reqbuf, 2); 

		//Waiting for message
		int sens_bytes = recv(sensors_in, buf, BUFFER_LEN, 0);
		if (sens_bytes == SOCKET_ERROR)
		{
			std::cout << "Error receiving from sensors " << WSAGetLastError() << endl;
			continue;
		}
		/*
		int req_bytes = recvfrom(request_in, reqbuf, 2, 0, (sockaddr*)&client, &clientLength);
		if (req_bytes == SOCKET_ERROR)
		{
			std::cout << "Error receiving from client " << WSAGetLastError() << endl;
			continue;
		}
		
		*/
		
		
		//Changing the address into humanly readable string
		char clientIp[256];
		ZeroMemory(clientIp, 256);
		inet_ntop(AF_INET, &client.sin_addr, clientIp, 256);



		//cout << "Recieved request from " << clientIp << " for " << (int)reqbuf[0] << ', ' << (int)reqbuf[1] << endl;
		
		//adding the value bytes to the uint64
		for (int i = BUFFER_LEN - 1; i > 1; i--) {
			ui_sens_64 = (ui_sens_64 << 8) + buf[i];
		}

		//Saving the text to the file
		save_to_txt((int)buf[0], (int)buf[1], ui_sens_64);

		//Adding the value to the corresponding sensor vector
		switch ((int)buf[0]) {
		case 0:
			if (sensor_0.type != (int)buf[1]) {			
				sensor_0.type = (int)buf[1];
			}
			add_to_last_10(sensor_0.vector, ui_sens_64);
			break;
		case 1:
			if (sensor_1.type != (int)buf[1]) {
				sensor_1.type = (int)buf[1];
			}
			add_to_last_10(sensor_1.vector, ui_sens_64);
			break;
		case 2:
			if (sensor_2.type != (int)buf[1]) {
				sensor_2.type = (int)buf[1];
			}
			add_to_last_10(sensor_2.vector, ui_sens_64);
			break;
		case 3:
			if (sensor_3.type != (int)buf[1]) {
				sensor_3.type = (int)buf[1];
			}
			add_to_last_10(sensor_3.vector, ui_sens_64);
			break;
		case 4:
			if (sensor_4.type != (int)buf[1]) {
				sensor_4.type = (int)buf[1];
			}
			add_to_last_10(sensor_4.vector, ui_sens_64);
			break;
		case 5:
			if (sensor_5.type != (int)buf[1]) {
				sensor_5.type = (int)buf[1];
			}
			add_to_last_10(sensor_5.vector, ui_sens_64);
			break;
		case 6:
			if (sensor_6.type != (int)buf[1]) {
				sensor_6.type = (int)buf[1];
			}
			add_to_last_10(sensor_6.vector, ui_sens_64);
			break;
		case 7:
			if (sensor_7.type != (int)buf[1]) {
				sensor_7.type = (int)buf[1];
			}
			add_to_last_10(sensor_7.vector, ui_sens_64);
			break;
		case 8:
			if (sensor_8.type != (int)buf[1]) {
				sensor_8.type = (int)buf[1];
			}
			add_to_last_10(sensor_8.vector, ui_sens_64);
			break;
		case 9:
			if (sensor_9.type != (int)buf[1]) {
				sensor_9.type = (int)buf[1];
			}
			add_to_last_10(sensor_9.vector, ui_sens_64);
			break;

		
		
			
		}

		//Checking the time
		end = chrono::system_clock::now();
		elapsed_seconds = end - start;

		//if more then 1 second has passed, then show the averages
		if (elapsed_seconds.count() > 1) {
			std::cout << "Sensor 0: " << get_v_ave(sensor_0.vector) << "\n";
			std::cout << "Sensor 1: " << get_v_ave(sensor_1.vector) << "\n";
			std::cout << "Sensor 2: " << get_v_ave(sensor_2.vector) << "\n";
			std::cout << "Sensor 3: " << get_v_ave(sensor_3.vector) << "\n";
			std::cout << "Sensor 4: " << get_v_ave(sensor_4.vector) << "\n";
			std::cout << "Sensor 5: " << get_v_ave(sensor_5.vector) << "\n";
			std::cout << "Sensor 6: " << get_v_ave(sensor_6.vector) << "\n";
			std::cout << "Sensor 7: " << get_v_ave(sensor_7.vector) << "\n";
			std::cout << "Sensor 8: " << get_v_ave(sensor_8.vector) << "\n";
			std::cout << "Sensor 9: " << get_v_ave(sensor_9.vector) << "\n" << "\n";
			start = chrono::system_clock::now(); //Start the timer again
		}
		

		
	}
	
	// Close socket
	closesocket(sensors_in);
	// Shutdown winsock
	WSACleanup();
	return 0;
}
