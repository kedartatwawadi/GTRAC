#ifndef INPUT_DATA_H_
#define INPUT_DATA_H_

class input_data {
public:
	input_data();
	input_data(char* path);

	int get_no_files();
	vector<string> get_file_names();
	int get_file_size();

private:
	int no_files;
	vector<string> file_names;
	int file_size;
	bool check_data(char* path);
};

#endif /* INPUT_DATA_H_ */