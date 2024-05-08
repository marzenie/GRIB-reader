#include <iostream>
#include <bitset>
#include <fstream>
#include "nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;
/*

https://www.nco.ncep.noaa.gov/pmb/docs/on388/
https://www.nco.ncep.noaa.gov/pmb/docs/on388/
https://www.nco.ncep.noaa.gov/pmb/docs/on388/
https://www.nco.ncep.noaa.gov/pmb/docs/on388/

Do czytania bajtów z pliku najlepiej użyć metody read(tablica, rozmiar);

*/
int main()
{
	fstream grib;
	//Otwarcie pliku w trybie do odczytu oraz trybie binarnym, koniecznie użyć binarnego or - | do złączenia tych flag
	grib.open("all.grib", std::ios::in | std::ios::binary);

	if (!grib.is_open()) {
		std::cout << "Nie udalo sie otworzyc pliku!\n";
		return 1;
	}

	char byte_char;
	uint32_t start_section_0 = 0;

	char byte;
	char byte2;
	char byte3;
	char byte4;

	std::string binary = "";

std::cout << R"(
========================== 
	
== General Message Info == 
		
==========================
 
)";

	//Szukamy "GRIB" w pliku
	while (!grib.eof())
	{
		grib.read(&byte, 1);
		//Jeżeli znalazłem G
		if (byte == 'G')
		{
			//Wczytaj kolejne 3 bajty
			grib.read(&byte2, 1);
			grib.read(&byte3, 1);
			grib.read(&byte4, 1);

			if (byte2 == 'R' && byte3 == 'I' && byte4 == 'B')
			{
				//I jeżeli są rowne RIB to znalazłem słowo GRIB
				std::cout << "Grib Finda at pos: " << (uint32_t)grib.tellg() - 4 << "\n";
				//Start sekcji 0 to aktualna pozycja (tellg)  - 4 bajty ( dlugosc słowa GRIB)
				start_section_0 = (uint32_t)grib.tellg() - 4;
				break;
			}
		}
	}

	int pos7777 = 0;
	//Szukamy 7777 oznaczajacego koniec pliku
	while (!grib.eof())
	{
		grib.read(&byte, 1);
		if (byte == '7')
		{
			grib.read(&byte2, 1);
			grib.read(&byte3, 1);
			grib.read(&byte4, 1);

			if (byte2 == '7' && byte3 == '7' && byte4 == '7')
			{
				pos7777 = (int)grib.tellg() - 4;
				std::cout << "7777 Find at position: " << pos7777 << "\n";
				break;
			}
		}
	}
	std::cout << "Distance bettwen end of grib and start of 7777: " << pos7777 - (int)start_section_0 + 4  << "\n";
	//Seekg - Przesun kursor na zadaną pozycje w pliku
	grib.seekg(start_section_0 + 4);
	int message_len = 0;

	binary = "";
	for (int i = 0; i < 3; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	message_len = std::stoi(binary, nullptr, 2);

	std::cout << "Whole Message length: " << message_len << "\n";

	std::cout << R"(
======================

==  Section 1 Data  ==

======================

)";
	//Pomijamy jeden byte
	grib.read(&byte_char, 1);


	uint32_t start_section_1 = grib.tellg();

	//Wczytujemy kolejno trzy bajty i umieszczamy je w 4 bajtowej zmiennej
	uint32_t section_1_length = 0;
	for (int i = 0; i < 3; ++i)
	{
		uint8_t tmp;
		grib >> tmp;

		//Z odpowiednim binarnym przesunięciem o wielokrotnosc 8, co daje nam kolejno 16, 8 , 0;
		section_1_length |= (static_cast<uint32_t>(tmp) << (8 * (2 - i)));
	}
	std::cout << "Section 1 length: " << section_1_length << "\n";

	grib.read(&byte_char, 1);
	std::cout << "Parameter table Version: " << (int)byte_char << "\n";

	grib.read(&byte_char, 1);
	std::ifstream centers_file("tables/table0.json");
	json centers_name = json::parse(centers_file)[0];
	string center_name = centers_name[to_string((int)byte_char)];
	std::cout << "Identification of Center: " << center_name << "\n";

	grib.read(&byte_char, 1);
	std::ifstream process_file("tables/tableA.json");
	json process_name = json::parse(process_file)[0];
	string proces_name = process_name[to_string((int)byte_char)];
	std::cout << "process ID number: " << proces_name << "\n";

	grib.read(&byte_char, 1);
	std::ifstream grid_ident_file("tables/tableB.json");
	json grid_ident = json::parse(grid_ident_file)[0];
	json grid_info = grid_ident[to_string((int)byte_char)];
	//string grid_info = grid_ident[to_string((int)byte_char)];
	std::cout << "Grid Identification info: (" << (int)byte_char << ")\n";
	std::cout << "  GRID: " << grid_info["GRID"] << "\n";
	std::cout << "  GRID_INCREMENT: " << grid_info["GRID_INCREMENT"] << "\n";


	grib.read(&byte_char, 1);
	string output_GDS_BMS = "";
	binary = std::bitset<8>((int)byte_char).to_string();
	for (std::string::size_type i = 0; i < binary.size(); ++i) {
		if (i == 0) {
			output_GDS_BMS += (binary[i] == 0) ? "GDS Omitted " : "GDS Included ";
			continue;
		}else if (i == 1) {
			output_GDS_BMS += (binary[i] == 0) ? "BMS Omitted" : "BMS Included";
			continue;
		}

		if (binary[i] == 1) {
			break;
		}

		if (i == binary.size() - 1) {
			output_GDS_BMS += " reserved";
		}
		
	}
	std::cout << "GDS and BMS: " << output_GDS_BMS << "\n";

	grib.read(&byte_char, 1);
	std::ifstream unit_parameters_file("tables/table2.json");
	json unit_parameters = json::parse(unit_parameters_file)[0];
	json unit_parameter = unit_parameters[to_string((int)byte_char)];

	std::cout << "Unit parameters: (" << (int)byte_char << ")\n";
	std::cout << "  PARAMETER: " << unit_parameter["PARAMETER"] << "\n";
	std::cout << "  UNITS: " << unit_parameter["UNITS"] << "\n";
	std::cout << "  ABBREV: " << unit_parameter["ABBREV"] << "\n";


	grib.read(&byte_char, 1);
	std::ifstream indicator_type_level_layer_file("tables/table3.json");
	json indicator_type_level_layer = json::parse(indicator_type_level_layer_file)[0];
	json indicator_tll = indicator_type_level_layer[to_string((int)byte_char)];

	std::cout << "Indicator of type of level or layer: (" << (int)byte_char << ")\n";
	std::cout << "  MEANING: " << indicator_tll["MEANING"] << "\n";
	std::cout << "  CONTENTS (Octets 11 & 12): " << indicator_tll["CONTENTS"] << "\n";
	std::cout << "  ABBREV: " << indicator_tll["ABBREV"] << "\n";

	binary = "";
	for (int i = 0; i < 2; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	std::cout << "Height, pressure, etc. of the level or layer: " << std::stoi(binary, nullptr, 2) << "\n";


	//grib.seekg(start_section_1 + 12);


	//grib >> byte >> byte2 >> byte3 >> byte4;




	//PRAWIDŁOWY SPOSOB NA ODCZYTYWANIE DANYCH Z PLIKU BINARNEGO
	std::cout << "YY/MM/DD|HH:MM : ";
	grib.read(&byte_char, 1);
	std::cout << int(byte_char) << "/";
	grib.read(&byte_char, 1);
	std::cout << int(byte_char) << "/";
	grib.read(&byte_char, 1);
	std::cout << int(byte_char);
	grib.read(&byte_char, 1);
	std::cout << ((std::to_string(byte_char).length()) == 1 ? "|0" : "|") << int(byte_char);
	grib.read(&byte_char, 1);
	std::cout << ((std::to_string(byte_char).length()) == 1 ? ":0" : ":") << int(byte_char) << "\n";

	grib.read(&byte_char, 1);
	std::ifstream time_unit_file("tables/table4.json");
	json time_units = json::parse(time_unit_file)[0];
	json time_unit = time_units[to_string((int)byte_char)];
	std::cout << "Time unit: (" << int(byte_char) << ") " << time_unit["TIME UNIT"] << "\n";

	grib.read(&byte_char, 1);
	std::cout << "P1 - Period of time: " << int(byte_char) << "\n";

	grib.read(&byte_char, 1);
	std::cout << "P2 - Period of time: " << int(byte_char) << "\n";

	grib.read(&byte_char, 1);
	std::cout << "Time range indicator: " << int(byte_char) << "\n";

	binary = "";
	for (int i = 0; i < 2; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	std::cout << "Numbers for last row: " << std::stoi(binary, nullptr, 2) << "\n";

	grib.read(&byte_char, 1);
	std::cout << "Number missing from averages or accumulation: " << int(byte_char) << "\n";

	grib.read(&byte_char, 1);
	std::cout << "Reference century: " << int(byte_char) << "\n";

	grib.read(&byte_char, 1);
	std::cout << "Identification of sub Center: " << int(byte_char) << "\n";

	binary = "";
	for (int i = 0; i < 2; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	std::cout << "Decimal Scale factor: " << std::stoi(binary, nullptr, 2) << "\n";

	std::cout << R"(
======================

==  Section 2 Data  ==

======================

)";
	grib.seekg(start_section_1 + section_1_length);
	uint32_t start_section_2 = grib.tellg();

	int section_2_length = 0;
	binary = "";
	for (int i = 0; i < 3; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	section_2_length = std::stoi(binary, nullptr, 2);
	std::cout << "Section 2 length: " << section_2_length << "\n";

	grib.read(&byte_char, 1);
	int vert_coord_nr = int(byte_char);
	std::cout << "Number of Vertical Coordinates: " << vert_coord_nr << "\n";

	grib.read(&byte_char, 1);
	int octet_nr = int(byte_char);
	std::cout << "Octet number: " << octet_nr << "\n";

	grib.read(&byte_char, 1);
	std::cout << "Data representation type: " << int(byte_char) << "\n";

	binary = "";
	for (int i = 0; i < 2; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	std::cout << "Numbers of points along a latitude circle: " << std::stoi(binary, nullptr, 2) << "\n";

	binary = "";
	for (int i = 0; i < 2; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	int num_points_along_longitude_meridian = std::stoi(binary, nullptr, 2);
	std::cout << "Numbers of points along a longitude meridian: " << num_points_along_longitude_meridian << "\n";

	binary = "";
	for (int i = 0; i < 3; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	int sign = binary[0] - '0';
	binary.erase(0, 1);
	std::cout << "Latitude:" << ((sign) ? " -" : " ") << std::stoi(binary, nullptr, 2) / 1000 << " degrees \n";

	binary = "";
	for (int i = 0; i < 3; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	sign = binary[0] - '0';
	binary.erase(0, 1);
	std::cout << "Longtitude:" << ((sign) ? " -" : " ") << std::stoi(binary, nullptr, 2) / 1000 << " degrees \n";

	grib.read(&byte_char, 1);
	string resolution_and_component_output = "";
	binary = std::bitset<8>((int)byte_char).to_string();

	for (std::string::size_type i = 0; i < binary.size(); ++i) {
		if (i == 0) {
			resolution_and_component_output += (binary[i] == '0') ? "Direction increments not given " : "Direction increments given ";
			continue;
		} else if (i == 1) {
			resolution_and_component_output += (binary[i] == '0') ? "Earth assumed spherical with radius = 6367.47 km " : "Earth assumed oblate spheroid with size as determined by IAU in 1965: 6378.160 km, 6356.775 km, f = 1/297.0 ";
			continue;
		} else if (i == 2) {
			resolution_and_component_output += ((binary[i] == '0') && (binary[i+1] == '0')) ? "reserved (set to 0) " : "";
			continue;
		} else if (i == 4) {
			resolution_and_component_output += (binary[i] == '0') ? "u- and v-components of vector quantities resolved relative to easterly and northerly directions " : "u and v components of vector quantities resolved relative to the defined grid in the direction of increasing x and y (or i and j) coordinates respectively ";
			continue;
		}

		if (binary[i] == 1) {
			break;
		}

		if (i == binary.size() - 1) {
			resolution_and_component_output += "reserved (set to 0)";
		}

	}
	std::cout << "Resolution and component flags: " << resolution_and_component_output << "\n";

	binary = "";
	for (int i = 0; i < 3; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	sign = binary[0] - '0';
	binary.erase(0, 1);
	std::cout << "Latitude of last grid point:" << ((sign) ? " -" : " ") << std::stoi(binary, nullptr, 2) / 1000 << " degrees \n";

	binary = "";
	for (int i = 0; i < 3; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	sign = binary[0] - '0';
	binary.erase(0, 1);
	std::cout << "Longitude of last grid point:" << ((sign) ? " -" : " ") << std::stoi(binary, nullptr, 2) / 1000 << " degrees \n";

	binary = "";
	for (int i = 0; i < 2; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	std::cout << "Longitudinal Direction Increment: " << std::stoi(binary, nullptr, 2) / 1000 << " degrees \n";

	binary = "";
	for (int i = 0; i < 2; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	std::cout << "Latitudinal Direction Increment: " << std::stoi(binary, nullptr, 2) / 1000 << " degrees \n";

	grib.read(&byte_char, 1);
	string scanning_mode_flags = "";
	bool i_pp;
	bool j_pp;
	bool I_priority;
	binary = std::bitset<8>((int)byte_char).to_string();

	for (std::string::size_type i = 0; i < binary.size(); ++i) {
		if (i == 0) {
			i_pp = (binary[i] == '1') ? true : false;
		} else if (i == 1) {
			j_pp = (binary[i] == '1') ? true : false;
		} else if (i == 2) {
			I_priority = (binary[i] == '0') ? true : false;
		}

	}
	// 28 - bit



	cout << "Scanning mode flags : Points scan in " << ((i_pp) ? "+i" : "-i") << " and " << ((j_pp) ? "+j" : "-j") << " direction: " << ((I_priority) ? "(FORTRAN: (I,J))" : "(FORTRAN: (J,I))") << "\n";

	for (int i = 1; (i + 28) < octet_nr; ++i) { // skip to map with points
		grib.read(&byte_char, 1);
	}

	for (int i = 1; i <= num_points_along_longitude_meridian; i++) {
		binary = "";
		int end = (vert_coord_nr === 0) ? 2 : 4;
		for (int j = 0; j < end; ++j) {
			grib.read(&byte_char, 1);
			binary += std::bitset<8>((int)byte_char).to_string();
		}
		cout << "Wiersz " << i << " Punktow: " << std::stoi(binary, nullptr, 2) << " \n";


	} 


	std::cout << R"(
======================

==  Section 3 Data  ==

======================

)";

	grib.seekg(start_section_2 + section_2_length);
	uint32_t start_section_3 = grib.tellg();

	binary = "";
	for (int i = 0; i < 3; ++i) {
		grib.read(&byte_char, 1);
		binary += std::bitset<8>((int)byte_char).to_string();
	}
	int section_3_length = std::stoi(binary, nullptr, 2);

	std::cout << "Section 3 length: " << std::stoi(binary, nullptr, 2) << "\n";

	grib.read(&byte_char, 1);
	std::cout << "Flag to decode: " << int(byte_char) << "\n";

	grib.read(&byte_char, 1);
	std::cout << "Binary Scale Factor: " << int(byte_char) << "\n";



	return 0;
}