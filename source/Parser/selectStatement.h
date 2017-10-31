#include "statement.h"

class selectStatement:public statement
{
public:
	void parse(parseTree* current);
	void parseFrom();
	void parseDistinct();
	void parseSelectList();
	void parseTableList();
	void parseOrderBy();
	bool checkSyntax();
	bool parseWhere();
	bool parseSearchCondition();


private:
	bool correctSyntactically;
};