#include "Executer.h"
void getTerminals(parseTree* node, std::vector<parseTree*>& terminals)
{
	if (node->getNodeType() == TERMINALS)
	{
		terminals.push_back(node);
		return;
	}
	vector<parseTree*> children = node->getChildren();
	for (vector<parseTree*>::iterator iter = children.begin(); iter != children.end(); iter++)
	{
		getTerminals(*iter, terminals);
	}
}

std::string getRelationName(std::vector<parseTree*>& terminals)
{
	for (vector<parseTree*>::iterator iter = terminals.begin(); iter != terminals.end(); iter++)
		if ((*iter)->getParent()->getNodeType() == TABLENAME)
			return (*iter)->getValue();
	return "";
}


std::vector<std::string> getFieldNames(std::vector<parseTree*>& terminals)
{
	std::vector<std::string> fieldNames;
	for (vector<parseTree*>::iterator iter = terminals.begin(); iter != terminals.end(); iter++)
	{
		if ((*iter)->getParent()->getNodeType() == ATTRIBUTENAME)
		{
			fieldNames.push_back((*iter)->getValue());
		}
	}
	return fieldNames;
}


std::vector<enum FIELD_TYPE> getFieldTypes(std::vector<parseTree*>& terminals)
{
	std::vector<enum FIELD_TYPE> fieldTypes;
	for (vector<parseTree*>::iterator iter = terminals.begin(); iter != terminals.end(); iter++)
	{
		if ((*iter)->getParent()->getNodeType() == DATATYPE)
		{
			if (!strcmp((*iter)->getValue(), "STR20"))
				fieldTypes.push_back(STR20);
			else if(!strcmp((*iter)->getValue(), "INT"))
				fieldTypes.push_back(INT);
		}
	}
	return fieldTypes;
}
std::map<std::string, std::string> getMapOfAttributes(std::vector<parseTree*>& terminals, bool& success)
{
	std::map<std::string, std::string> mapOfAttributes;
	std::vector<std::string> attributes, values;
	for (vector<parseTree*>::iterator iter = terminals.begin(); iter != terminals.end(); iter++)
	{
		if ((*iter)->getParent()->getNodeType() == ATTRIBUTENAME)
		{
			attributes.push_back((*iter)->getValue());
		}
		else if (((*iter)->getParent()->getParent() != NULL) && (*iter)->getParent()->getParent()->getNodeType() == VALUE)
		{
			values.push_back((*iter)->getValue());
		}
	}
	if (attributes.size() != values.size())
	{
		success = false;
		return mapOfAttributes;
	}
	for (int count = 0; count < attributes.size(); count++)
	{
		mapOfAttributes.insert(std::make_pair(attributes[count], values[count]));
	}
	success = true;
	return mapOfAttributes;
}

bool isSimpleInsert(std::vector<parseTree*>& terminals)
{
	for (vector<parseTree*>::iterator iter = terminals.begin(); iter != terminals.end(); iter++)
	{
		if ((*iter)->getParent()->getNodeType() == SELECTSTATEMENT)
			return false;
	}
	return true;
}


void executeQuery(SchemaManager& schemaManagerObj, MainMemory& mainMemoryObj, Disk& diskObj, statement* statementObj, std::ofstream& fWriteExecute)
{
	std::vector<parseTree*> terminals;
	parseTree* pTree = statementObj->getRoot();
	getTerminals(pTree, terminals);
	if (pTree->getNodeType() == CREATETABLESTATEMENT)
	{
		std::string relation_name = getRelationName(terminals);
		std::vector<std::string> field_names = getFieldNames(terminals);
		std::vector<enum FIELD_TYPE> field_types = getFieldTypes(terminals);
		Relation * relationObj = createTable(schemaManagerObj, relation_name, field_names, field_types);
		if (!relationObj)
			fWriteExecute << "Execute of create failed\n\n\n\n\n";
		else
			fWriteExecute << "Create was successful\n\n" << schemaManagerObj << "\n\n\n\n\n";
	}
	else if (pTree->getNodeType() == DROPTABLESTATEMENT)
	{
		std::string relation_name = getRelationName(terminals);
		bool retVal = dropTable(relation_name, fWriteExecute, schemaManagerObj);
		if (!retVal)
			fWriteExecute << "Execute of drop failed\n\n\n\n\n";
		else
			fWriteExecute << "Drop succeeded\n\n\n\n\n";
	}
	else if (pTree->getNodeType() == INSERTSTATEMENT)
	{
		std::string relation_name = getRelationName(terminals);
		bool success = false;
		std::map<std::string, std::string> mapOfAttributes;
		if (isSimpleInsert(terminals))
		{
			mapOfAttributes = getMapOfAttributes(terminals, success);
			if (!success)
			{
				fWriteExecute << "Execute of insert failed\n\n\n\n\n";
				return;
			}
		}
		bool retVal = insertTable(relation_name, schemaManagerObj, mapOfAttributes, mainMemoryObj);
		if (!retVal)
			fWriteExecute << "Execute of insert failed\n\n\n\n\n";
		else
			fWriteExecute << "Insert succeeded\n\n" << schemaManagerObj<<"\n\n\n\n\n";
	}
}