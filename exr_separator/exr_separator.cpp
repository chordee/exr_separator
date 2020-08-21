// exr_separator.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <string>
#include <vector>
#include <typeinfo>
#include <filesystem>
#include <map>
#include <regex>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

using namespace OIIO;
using namespace ImageBufAlgo;

using namespace std;


vector<string> split(const string &str, string delim)
{
	vector<string> result;
	string::size_type pos = str.find(delim);
	result.push_back(str.substr(0, pos));
	result.push_back(str.substr(pos + 1));
	return result;
};

void separate_exr(const string &filename, const bool &extBasename = 0, const float &zNormalize = 1, string z_channel_name = "")
{
	ImageBuf &in = ImageBuf(filename);
	int nsubimages = in.nsubimages();

	filesystem::path &path = filesystem::path(filename);
	const filesystem::path &source_file = path.filename();
	const auto source_names = split(source_file.string(), ".");

	vector<map<string, vector<int>>> subimages;
	// Gernerate Channel Map in Subimages Vector
	for (int j = 0; j < nsubimages; ++j) {
		in.read(j);
		const ImageSpec &spec = in.spec();
		int nchannels = spec.nchannels;
		map<string, vector<int>> channel_map;

		for (int i = 0; i < nchannels; ++i) {
			const string &ch_name = spec.channelnames[i];
			if (ch_name.size() > 0) {
				auto splitted = split(ch_name, ".");
				auto iter = channel_map.find(splitted[0]);
				if (iter == channel_map.end()) {
					channel_map.insert(pair<string, vector<int>>(splitted[0], { i }));
				}
				else {
					channel_map[splitted[0]].push_back(i);
				}
			}
		}
		subimages.push_back(channel_map);
	}

	//for (auto &k : subimages) {
	//	for (auto &kk : k) {
	//		cout << kk.first;
	//		for (auto &kkk : kk.second) {
	//			cout << "::" << kkk;
	//		}
	//		cout << endl;
	//	}
	//	cout << "====" << endl;
	//}

	for (int j = 0; j < nsubimages; ++j) {
		in.read(j);
		const ImageSpec &spec = in.spec();
		int nchannels = spec.nchannels;
		auto channel_map = subimages[j];

		vector<string> main_channels({ "R","G","B","A" });
		vector<int> tmp;

		for (auto &ch : main_channels) {
			auto p = find(spec.channelnames.begin(), spec.channelnames.end(), ch);
			if (p == spec.channelnames.end())
				break;
			tmp.push_back((int)distance(spec.channelnames.begin(), p));
		}
		if (!tmp.empty()) {
			string main_channel_name = "beauty";
			if (spec.find_attribute("name"))
				main_channel_name = spec.find_attribute("name")->get_string();
			channel_map.insert(make_pair(main_channel_name, tmp));

			for (auto &ch : main_channels)
				channel_map.erase(ch);
		}
		if (nchannels == 1) {
			channel_map.clear();
			vector<int> tmp({ 0 });
			channel_map.insert(make_pair(spec.find_attribute("name")->get_string(), tmp));
		}

		// Find Z-Depth Channel
		if (spec.z_channel != -1)
			z_channel_name = spec.channelnames[spec.z_channel];

		for (auto &it : channel_map) {
			if (it.second.size() > 0) {
				auto dir = path.parent_path().append(it.first);
				if (nsubimages > 1) {
					dir = path.parent_path().append(spec.find_attribute("name")->get_string());
				}
				filesystem::create_directory(dir);
				filesystem::path dst;
				if (extBasename == 0) {
					dst = dir.append(source_names[0] + "." + source_names[1]);
				}
				else {
					dst = dir.append(source_names[0] + "_" + it.first + "." + source_names[1]);
				}
				ImageBuf out;
				int *order;
				if (it.second.size() > 0) {
					if (it.first == "N") {
						auto p0 = find(spec.channelnames.begin(), spec.channelnames.end(), "N.x");
						auto p1 = find(spec.channelnames.begin(), spec.channelnames.end(), "N.y");
						auto p2 = find(spec.channelnames.begin(), spec.channelnames.end(), "N.z");
						int tmp[] = { (int)distance(spec.channelnames.begin(), p0), (int)distance(spec.channelnames.begin(), p1), (int)distance(spec.channelnames.begin(), p2) };
						order = &tmp[0];
					}
					else {
						order = &(it.second[0]);
					}
					vector<string> names = { "R", "G", "B", "A" };
					vector<float> values = { 0, 0, 0, 0 };
					names.resize(it.second.size());
					values.resize(it.second.size());
					ImageBufAlgo::channels(out, in, it.second.size(), order, &values[0], &names[0]);
					if (!z_channel_name.empty() && it.first == z_channel_name) {
						ImageBufAlgo::mul(out, out, 1 / zNormalize);
					}
					out.write(dst.string());

				}
			}
		}
	}
};

// main 

int main(int argc, char **argv)
{
	vector<string> v_argv(argv, argv + argc);

	int extbase = 0;
	float zN = 1;
	string z_channel_name;

	for (auto &it : vector<string>({ "-help", "--help", "-h" })) {
		auto ptr = find(v_argv.begin(), v_argv.end(), it);
		if (ptr != v_argv.end()) {
			cout << "-zN <number> \t Normalize Z-Depth" << endl\
				<< "-zName <name> \t Z-Depth Channel Name" << endl \
				<< "-extBase \t Suffix file basename with channel name" << endl;
			return 1;
		}
	}

	auto ptr = find(v_argv.begin(), v_argv.end(), "-extBase");
	if (ptr != v_argv.end()) {
		extbase = 1;
		v_argv.erase(ptr);
	}

	ptr = find(v_argv.begin(), v_argv.end(), "-zN");
	if (ptr != v_argv.end()) {
		int index = distance(v_argv.begin(), ptr);
		regex re("^[0-9\.]*$");
		if (!regex_match(v_argv[index + 1], re)) {
			cout << "-zN ???(float)" << endl;
			cout << "Wrong argument" << endl;
			return 1;
		}
		zN = stof(v_argv[index + 1]);
		v_argv.erase(ptr, ptr + 1);
	}

	ptr = find(v_argv.begin(), v_argv.end(), "-zName");
	if (ptr != v_argv.end()) {
		int index = distance(v_argv.begin(), ptr);
		z_channel_name = v_argv[index + 1];
		v_argv.erase(ptr, ptr + 1);
	}

	for (int i = 1; i < v_argv.size(); ++i) {
		filesystem::path path(v_argv[i]);
		if (filesystem::exists(path)) {
			if (!filesystem::is_directory(path)) {
				if (path.extension() == ".exr") {
					cout << "exr File Reading..." << endl;
					separate_exr(filesystem::absolute(path).string(), extbase, zN, z_channel_name);
				}
			}
			else {
				for (auto &it : filesystem::directory_iterator(path)) {
					if (!filesystem::is_directory(it.path()) && it.path().extension() == ".exr") {
						cout << it.path() << " ..." << endl;
						separate_exr(filesystem::absolute(it.path()).string(), extbase, zN, z_channel_name);
					}
				}
			}
		}
	}
	return 0;
}
