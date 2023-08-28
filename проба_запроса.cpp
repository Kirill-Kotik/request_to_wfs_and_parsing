

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <pugixml.hpp>
#include <map>
#include <sstream>
#include <vector>

using namespace std;

size_t write_data(char* ptr, size_t size, size_t nmemb, void* userdata) {
	size_t written = fwrite(ptr, size, nmemb, (FILE*)userdata);
	return written;
}
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* user_data) {
	size_t total_size = size * nmemb;
	string* response = static_cast<string*>(user_data);
	response->append(static_cast<char*>(contents), total_size);
	return total_size;
}

int main()
{
	setlocale(LC_ALL, "Russian");
	CURL* curl;
	CURLcode res;
	static const char* outfile = "out.txt";
	FILE* pagefile;


	//string typeName("geosmr:SM_590_57");
	//string GetFeature = "?version=1.0.0&request=GetFeature&typeName=" + typeName + "&srsName=EPSG:4326";

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	string response_buffer;

	if (curl) {

		string wfs_url = "https://map.samadm.ru/wfs1";
		string info = "?SERVICE=WFS&VERSION=1.0.0&REQUEST=GetFeature";
		string add = "&TYPENAME=geosmr:SM_590_60&SRSNAME=EPSG:4326";
		string full_url = wfs_url + info + add;

		cout << full_url << endl;

		curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			cerr << "Failed to perform GetFeature request: " << curl_easy_strerror(res) << endl;
		}
		else {
			pugi::xml_document doc;
			auto result = doc.load_string(response_buffer.c_str());
			//auto result=doc.load_file("out.txt");
			

			if (result) {
				map<int, vector<pair<double, double>>> mp;
				map<int, string >m4plg;
				string path = "/wfs:FeatureCollection/gml:featureMember/geosmr:SM_590_60";
				pugi::xpath_node_set featureMembers = doc.select_nodes(path.c_str());

				for (pugi::xpath_node featureMemberNode : featureMembers) {
					int idNode = stoi(featureMemberNode.node().child_value("geosmr:id"));
					if (idNode) {
						pugi::xml_node geometryNode = featureMemberNode.node().select_node("geometry/gml:MultiGeometry/gml:geometryMember").node();
						pugi::xml_node childNode = geometryNode.first_child();
						string childTagName = childNode.name();


						if (childTagName == "Point") {
							cout << "Point case" << endl;
							for (pugi::xpath_node featureMemberNode : featureMembers) {
								int idNode = stoi(featureMemberNode.node().child("geosmr:id").child_value());
								pugi::xpath_node_set posListNodes = featureMemberNode.node().select_nodes("geometry/gml:MultiGeometry/gml:geometryMember/Point");
								for (pugi::xpath_node posListNode : posListNodes) {
									string posListText = posListNode.node().child("pos").child_value();

									vector<pair<double, double>> coordinates;
									istringstream ss(posListText);
									double x, y;
									while (ss >> x >> y) {
										coordinates.push_back(make_pair(x, y));
									}
									mp[idNode] = coordinates;

								}
							}
							for (const auto& it : mp) {
								cout << "ID: " << it.first << endl;
								for (const auto& coord : it.second) {
									cout << "X: " << setprecision(15) << coord.first << " Y: " << setprecision(15) << coord.second << endl;
								}
							}
						}
						else if (childTagName == "Polygon") {
							cout << "Polygon case" << endl;
							for (pugi::xpath_node featureMemberNode : featureMembers) {
								int idNode = stoi(featureMemberNode.node().child("geosmr:id").child_value());
								pugi::xpath_node_set posListNodes = featureMemberNode.node().select_nodes("geometry/gml:MultiGeometry/gml:geometryMember/Polygon/exterior/LinearRing");
								for (pugi::xpath_node posListNode : posListNodes) {
									string posListText = posListNode.node().child("posList").child_value();

									//m4plg[idNode] = posListText;
									cout << "ID: " << idNode << endl;
									cout << "POLYGON:  " << posListText << endl;
								}
							}
							
						}

					}

				}
				
			}
			else {
				cerr << "pugixml parsing error: " << result.description() << endl;
			}
		}
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();

	return 0;
}