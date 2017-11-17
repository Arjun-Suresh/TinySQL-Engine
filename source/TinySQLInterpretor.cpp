// TinySQLInterpretor.cpp : Defines the entry point for the console application.
#include "TinySQLInterpretor.h"
#define SYNTAXVALIDITYFILE "OutputResults.txt"
#define EXECUTERESULTSFILE "ResultsExecution.txt"

SchemaManager *schemaManagerObj = NULL;
MainMemory* mainMemoryObj = NULL;
Disk* diskObj = NULL;

void initMemory()
{
	mainMemoryObj = new MainMemory();
	diskObj = new Disk();
	schemaManagerObj = new SchemaManager(mainMemoryObj, diskObj);
}

statement* getParserObject(std::string line)
{
	statement* obj = NULL;
	int index = 0;
	if (matchString(line, index, "select"))
	{
		selectStatement* sObj = new selectStatement();;
		obj = sObj;
	}
	else if (matchString(line, index, "create"))
	{
		createStatement* cObj = new createStatement();
		obj = cObj;
	}
	else if (matchString(line, index, "delete"))
	{
		deleteStatement* dObj = new deleteStatement();
		obj = dObj;
	}
	else if (matchString(line, index, "insert"))
	{
		insertStatement* iObj = new insertStatement();
		obj = iObj;
	}
	else if (matchString(line, index, "drop"))
	{
		dropStatement* dObj = new dropStatement();
		obj = dObj;
	}
	return obj;
}
char* getNewBlock(int maxCount, int count, char* line)
{
	char* newBlock = new char[maxCount * 2];
	for (int i = 0; i < count; i++)
		newBlock[i] = line[i];
	delete[] line;
	return newBlock;
}

std::string readLinesFromFile(std::ifstream* input)
{
	char* line = new char[100];
	int maxCount = 100;
	int count = 0;
	char c;
	input->get(c);
	while (c != '\n' && !(input->eof()))
	{
		line[count++] = c;
		input->get(c);
		if (count == maxCount-5)
		{
			line = getNewBlock(maxCount, count, line);
			maxCount = maxCount * 2;
		}
	}
	line[count++] = '\n';
	line[count] = '\0';
	return std::string(line);
}

std::string readLine()
{
	char* line;
	std::string lineString;
	int maxCount = 100;
	int count = 0;
	std::getline(std::cin, lineString);
	line = new char[lineString.length() + 1];
	strcpy_s(line, lineString.length() + 1, lineString.c_str());
	return std::string(line);
}

std::string readGeneric(std::ifstream* input)
{
	if (!input)
		return readLine();
	else
		return readLinesFromFile(input);
}

bool checkIfCommandToBeRead(std::ifstream* input, bool readFlag)
{
	bool retValue = true;
	if (!input && readFlag)
		retValue = false;
	else if (input && input->eof())
		retValue = false;
	return retValue;
}

int main()
{
	int option;
	bool readFlag = false;
	std::ifstream fPtr, *input;
	initMemory();
	std::cout << "1. Single SQL command from standard input\n2. Multiple SQL commands from a file\n";
	std::cin >> option;
	while (option != 1 && option != 2)
	{
		std::cout << "Invalid selection\n";
		std::cout << "1. Single SQL command from standard input\n2. Multiple SQL commands from a file\n";
		std::cin >> option;
	}
	if (option == 2)
	{
		char ch = getchar();
		std::cout << "Enter the path of the file with the SQL commands\n";
		std::string filePath;
		std::getline(std::cin, filePath);
		fPtr.open(filePath.c_str(), std::ifstream::in);
		if (!(fPtr.good()))
		{
			std::cout << "File doesn't exist\n";
			return 0;
		}
		input = &fPtr;		
	}
	else
	{
		input = NULL;
	}
	
	std::remove(SYNTAXVALIDITYFILE);
	std::ifstream test(SYNTAXVALIDITYFILE);
	if (test.good())
	{
		std::cout << "Please try closing the output result file and running the application\n";
		return 0;
	}
	std::ofstream fWrite(SYNTAXVALIDITYFILE);

	std::remove(EXECUTERESULTSFILE);
	std::ifstream testExecute(EXECUTERESULTSFILE);
	if (testExecute.good())
	{
		std::cout << "Please try closing the execute result file and running the application\n";
		return 0;
	}
	std::ofstream fWriteExecute(EXECUTERESULTSFILE);
	if(!input)
		std::cin.ignore(1000, '\n');
	while (checkIfCommandToBeRead(input, readFlag))
	{
		if(!input)
			std::cout << "Enter the statement\n";
		int index = 0;
		std::string line = readGeneric(input);
		statement* obj = getParserObject(line);
		if (!obj)
		{
			fWrite << "Invalid statement\n";
			break;
		}
		else
		{
			obj->parse(line, index);
			if (obj->isValidSyntax())
			{
				fWrite << "Syntax valid\n";
				executeQuery((*schemaManagerObj), (*mainMemoryObj), (*diskObj), obj, fWriteExecute);
			}

			else
			{
				fWrite << "Invalid syntax\n";
				break;
			}
		}
		if (!input)
		{
			std::cout << "Enter q to quit or any other character to enter another command\n";
			char ch = getchar();
			if (ch == 'q' || ch == 'Q')
				readFlag = true;
			else
				ch = getchar();
		}
	}
	fWrite.close();
	fWriteExecute.close();
	char chRead = getchar();
}