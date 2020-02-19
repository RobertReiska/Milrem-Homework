#include <iostream>
#include <WS2tcpip.h>
#include <fstream>
#include <vector>
#include <string>
#include <numeric>
#include <chrono>
#include <ctime>
#include <sstream>

#pragma comment (lib, "ws2_32.lib") //Including library for networking in Windows


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

//Gets the average of all values in "Sensor data.txt"
uint64_t get_ave(int sensor) {

	uint64_t	average = 0;
	int			nr_of_readings = 0, multiplier = 1;

	string				line, value, sens_nr, value_type;

	ifstream file;

	file.open("Sensor data.txt", ios::in);

	while (getline(file, line)) {				//getting the line fro file
		stringstream ss(line);					//breaking it down to words
		getline(ss, sens_nr, ',');

		if (stoi(sens_nr) == sensor) {			//seeing if itsthe reading from the sensor
       
			getline(ss, value_type, ',');

			if (stoi(sens_nr) >= 3) {
				getline(ss, value, ',');
				multiplier = 10000;
			}

			average += stoll(value)/ multiplier;	//multiplier in the case uint64 overflows
			nr_of_readings += 1;
		}

	}
	file.close();
	if (nr_of_readings != 0) return average / nr_of_readings* multiplier;
	else return average;
	
}


int main()
{

	char				buf[BUFFER_LEN], sendbuf[BUFFER_LEN], reqbuf[2];		//Buffer for storing incoming sensor data, sending data and request data
	uint64_t			sendValue = 0, ui_sens_64 = 0;						//Int for storing the value of the sensor and sending the value
	chrono::time_point<chrono::system_clock>	start, end;					//Timepoints for outputting at 1 Hz
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

	//Creating UDP sockets, one for each port
	SOCKET sensors_in = socket(AF_INET, SOCK_DGRAM, 0);

	//Setting up server address and port
	sockaddr_in sensors_in_hint;
	sensors_in_hint.sin_addr.S_un.S_addr = ADDR_ANY;	//Any address
	sensors_in_hint.sin_family = AF_INET;				//IPv4
	sensors_in_hint.sin_port = htons(12345);			//assigning port with big endian

	//Binding it to socket
	if (bind(sensors_in, (sockaddr*)&sensors_in_hint, sizeof(sensors_in_hint)) == SOCKET_ERROR)
	{
		std::cout << "Can't bind socket! " << WSAGetLastError() << endl;
		return 1;
	}
	

	//Same for second socket
	SOCKET request_in = socket(AF_INET, SOCK_DGRAM, 0);

	sockaddr_in request_in_hint;
	request_in_hint.sin_addr.S_un.S_addr = ADDR_ANY;	
	request_in_hint.sin_family = AF_INET;				
	request_in_hint.sin_port = htons(12346);			

	if (bind(request_in, (sockaddr*)&request_in_hint, sizeof(request_in_hint)) == SOCKET_ERROR)
	{
		std::cout << "Can't bind socket! " << WSAGetLastError() << endl;
		return 1;
	}

	//Maiking socket set for select()
	fd_set sockets;
	FD_ZERO(&sockets);
	FD_SET(sensors_in, &sockets);
	FD_SET(request_in, &sockets);

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
		ZeroMemory(sendbuf, BUFFER_LEN); 
		ZeroMemory(reqbuf, 2);
		ui_sens_64 = 0;
		sendValue = 0;

		//Seeing if any of the sockets are recieving data
		if (select(3, &sockets, (fd_set*)0, (fd_set*)0, 0) >= 0) {
		if (select(3, &sockets, (fd_set*)0, (fd_set*)0, 0) >= 0) {
			
			//If were getting data from sensors...
			if (FD_ISSET(sensors_in, &sockets)) {						

				if (int sens_bytes = recv(sensors_in, buf, BUFFER_LEN, 0) == SOCKET_ERROR)
				{
					std::cout << "Error receiving from sensors " << WSAGetLastError() << endl;
					continue;
				}

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

				
			}

			//If were getting requests...
			if (FD_ISSET(request_in, &sockets)) {
				
				if (int req_bytes = recvfrom(request_in, reqbuf, 2, 0, (sockaddr*)&client, &clientLength) == SOCKET_ERROR)
				{
					std::cout << "Error receiving from client " << WSAGetLastError() << endl;
					continue;
				}

				//Which sensor is requested
				switch ((int)buf[0]) {
				case 0:
					sendbuf[0] = (int)buf[0];											//first byte of packet is sensor number
					sendbuf[1] = sensor_0.type;											//second byte of packet is data type
					if ((int)buf[1] == 0) sendValue = sensor_0.vector[0];				//last bytes are the sensor data
					else if ((int)buf[1] == 1) sendValue = get_v_ave(sensor_0.vector);
					else if ((int)buf[1] == 2) sendValue = get_ave((int)buf[0]);
					break;
				case 1:
					sendbuf[0] = (int)buf[0];
					sendbuf[1] = sensor_1.type;
					if ((int)buf[1] == 0) sendValue = sensor_1.vector[0];
					else if ((int)buf[1] == 1) sendValue = get_v_ave(sensor_1.vector);
					else if ((int)buf[1] == 2) sendValue = get_ave((int)buf[0]);
					break;
				case 2:
					sendbuf[0] = (int)buf[0];
					sendbuf[1] = sensor_2.type;
					if ((int)buf[1] == 0) sendValue = sensor_2.vector[0];
					else if ((int)buf[1] == 1) sendValue = get_v_ave(sensor_2.vector);
					else if ((int)buf[1] == 2) sendValue = get_ave((int)buf[0]);
					break;
				case 3:
					sendbuf[0] = (int)buf[0];
					sendbuf[1] = sensor_3.type;
					if ((int)buf[1] == 0) sendValue = sensor_3.vector[0];
					else if ((int)buf[1] == 1) sendValue = get_v_ave(sensor_3.vector);
					else if ((int)buf[1] == 2) sendValue = get_ave((int)buf[0]);
					break;
				case 4:
					sendbuf[0] = (int)buf[0];
					sendbuf[1] = sensor_4.type;
					if ((int)buf[1] == 0) sendValue = sensor_4.vector[0];
					else if ((int)buf[1] == 1) sendValue = get_v_ave(sensor_4.vector);
					else if ((int)buf[1] == 2) sendValue = get_ave((int)buf[0]);
					break;
				case 5:
					sendbuf[0] = (int)buf[0];
					sendbuf[1] = sensor_5.type;
					if ((int)buf[1] == 0) sendValue = sensor_5.vector[0];
					else if ((int)buf[1] == 1) sendValue = get_v_ave(sensor_5.vector);
					else if ((int)buf[1] == 2) sendValue = get_ave((int)buf[0]);
					break;
				case 6:
					sendbuf[0] = (int)buf[0];
					sendbuf[1] = sensor_6.type;
					if ((int)buf[1] == 0) sendValue = sensor_6.vector[0];
					else if ((int)buf[1] == 1) sendValue = get_v_ave(sensor_6.vector);
					else if ((int)buf[1] == 2) sendValue = get_ave((int)buf[0]);
					break;
				case 7:
					sendbuf[0] = (int)buf[0];
					sendbuf[1] = sensor_7.type;
					if ((int)buf[1] == 0) sendValue = sensor_7.vector[0];
					else if ((int)buf[1] == 1) sendValue = get_v_ave(sensor_7.vector);
					else if ((int)buf[1] == 2) sendValue = get_ave((int)buf[0]);
					break;
				case 8:
					sendbuf[0] = (int)buf[0];
					sendbuf[1] = sensor_8.type;
					if ((int)buf[1] == 0) sendValue = sensor_8.vector[0];
					else if ((int)buf[1] == 1) sendValue = get_v_ave(sensor_8.vector);
					else if ((int)buf[1] == 2) sendValue = get_ave((int)buf[0]);
					break;
				case 9:
					sendbuf[0] = (int)buf[0];
					sendbuf[1] = sensor_9.type;
					if ((int)buf[1] == 0) sendValue = sensor_9.vector[0];
					else if ((int)buf[1] == 1) sendValue = get_v_ave(sensor_9.vector);
					else if ((int)buf[1] == 2) sendValue = get_ave((int)buf[0]);
					break;




				}
				
				//Copying the bytes from uint64 to packet
				std::memcpy(sendbuf+2, &sendValue, BUFFER_LEN-2);

				//Sending packet back
				if (int send_bytes = sendto(request_in, sendbuf, BUFFER_LEN, 0, (sockaddr*)&client, clientLength) == SOCKET_ERROR)
				{
					std::cout << "Error sending to client " << WSAGetLastError() << endl;
					continue;
				}


			}
		}
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
	
	// Closing sockets
	closesocket(sensors_in);
	closesocket(request_in);

	// Shutdown winsock
	WSACleanup();
	return 0;
}
