

class input_data {
private:
	/* data */
	int file_size;
	int number_of_files;
	vector<string> file_names;
	unsigned char* reference_file;

public:
	input_data() = default;
	void input_data(char* input_dir);
	void 

};