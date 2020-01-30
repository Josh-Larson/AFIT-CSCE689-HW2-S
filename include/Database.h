#pragma once

#include <functional>
#include <array>
#include <optional>
#include <utility>
#include <fcntl.h>
#include <unistd.h>
#include <climits>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

template<int columns, char delimeter = ','>
class Database {
	const std::string filename;
	
	public:
	explicit Database(std::string filename) : filename(std::move(filename)) {}
	~Database() = default;
	
	using DatabaseRow = std::array<std::string, columns>;
	using FindDatabaseRowFunction = const std::function<bool(const DatabaseRow &)> &;
	using ReadDatabaseRowFunction = const std::function<bool(const DatabaseRow &)> &;
	using UpdateDatabaseRowFunction = const std::function<DatabaseRow(const DatabaseRow &)> &;
	
	std::optional<DatabaseRow> find(FindDatabaseRowFunction op) {
		std::optional<DatabaseRow> ret = std::nullopt;
		readFromFile([&](const DatabaseRow& data) {
			if (op(data)) {
				ret = data;
				return false;
			}
			return true;
		});
		return ret;
	}
	
	bool update(UpdateDatabaseRowFunction op) {
		return updateFile([&](int fd) {
			return readFromFile([&](const DatabaseRow & row) {
				writeToFile(fd, op(row));
				return true;
			}) == 0;
		});
	}
	
	bool insert(const DatabaseRow & data) {
		return updateFile([&](int fd) {
			readFromFile([&](const DatabaseRow & row) { writeToFile(fd, row); return true; });
			writeToFile(fd, data);
			return true;
		});
	}
	
	private:
	int readFromFile(ReadDatabaseRowFunction op) {
		std::array<char, 1024> buffer{};
		DatabaseRow data{};
		std::vector<char> stringBuffer;
		int currentColumn = 0;
		bool escaped = false;
		
		int fd = open(filename.c_str(), 0, O_RDONLY);
		if (fd < 0)
			return -1;
		
		bool terminateRead = false;
		int n;
		while (!terminateRead && (n = read(fd, buffer.data(), buffer.size())) > 0) {
			for (int i = 0; i < n; i++) {
				switch (buffer[i]) {
					case '\\':
						if (escaped)
							stringBuffer.push_back('\\');
						escaped = !escaped;
						break;
					case delimeter:
						if (!escaped) {
							data[currentColumn] = std::string(&stringBuffer[0], stringBuffer.size());
							currentColumn++;
							stringBuffer.clear();
							break;
						}
					default:
						stringBuffer.push_back(buffer[i]);
						escaped = false;
						break;
					case '\r':
					case '\n':
						if (currentColumn+1 != columns) {
							currentColumn = 0;
							escaped = false;
							stringBuffer.clear();
							break;
						}
						data[currentColumn] = std::string(&stringBuffer[0], stringBuffer.size());
						if (!op(data))
							terminateRead = true;
						stringBuffer.clear();
						currentColumn = 0;
						escaped = false; // We don't escape newlines
						break;
				}
			}
		}
		close(fd);
		return 0;
	}
	
	bool updateFile(const std::function<bool(int fd)> & op) {
		auto tmpfile = filename + ".tmp";
		remove(tmpfile.c_str());
		int fd = open(tmpfile.c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR);
		if (!op(fd)) {
			close(fd);
			return false;
		}
		
		if (fchmod(fd, S_IRUSR | S_IRGRP | S_IROTH) != 0) {
			fprintf(stderr, "Failed to change temp file to read-only: %s\n", strerror(errno));
			return false;
		}
		
		close(fd);
		if (rename(tmpfile.c_str(), filename.c_str()) != 0) {
			fprintf(stdout, "Failed to copy over temp file: %s\n", strerror(errno));
			return false;
		}
		return true;
	}
	
	void writeToFile(int fd, const DatabaseRow & data) {
		std::array<char, 1> comma = {delimeter};
		int columnIndex = 0;
		for (const std::string & col : data) {
			if (columnIndex > 0)
				write(fd, comma.data(), 1);
			write(fd, col.c_str(), col.length());
			columnIndex++;
		}
		write(fd, "\n", 1);
	}
	
};