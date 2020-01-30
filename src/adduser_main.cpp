/****************************************************************************************
 * my_adduser_main - creates a user account and password from the command prompt
 *
 *              **Students should not modify this code! Or at least you can to test your
 *                code, but your code should work with the unmodified version
 *
 ****************************************************************************************/

#include <stdexcept>
#include <iostream>
#include <Database.h>
#include <Security.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stdout, "%s <username>\n", argv[0]);
		return 0;
	}
	
	std::string username(argv[1]);
	if (username.find(',') != std::string::npos) {
		fprintf(stderr, "Usernames cannot have a comma in them.\n");
		return -1;
	}
	
	Database<3, ','> passwd("passwd");
	if (passwd.find([&](const auto &row) { return row[0] == username; })) {
		fprintf(stderr, "That user already has an account.\n");
		return -1;
	}
	
	Security::INSTANCE()->setFDEcho(0, false);
	
	while (true) {
		std::string passwd1, passwd2;
		fprintf(stdout, "\nAdding user\nNew Password: ");
		fflush(stdout);
		std::cin >> passwd1;
		
		fprintf(stdout, "\nEnter the password again: ");
		fflush(stdout);
		std::cin >> passwd2;
		
		if (passwd1 == passwd2) {
			Security::INSTANCE()->setFDEcho(0, true);
			auto salt = Security::INSTANCE()->generateSalt();
			if (passwd.insert({username, salt, Security::INSTANCE()->hash(passwd1, salt)})) {
				fprintf(stdout, "\nUser added.\n");
			} else {
				fprintf(stderr, "\nFailed to add user. Unknown error.\n");
				return -1;
			}
			
			break;
		} else {
			fprintf(stdout, "\nPasswords do not match. Try again.\n");
		}
	}
	
	return 0;
}
