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

bool singleTableStatement(std::vector<parseTree*>& terminals)
{
	vector<parseTree*>::iterator iter;
	for (iter = terminals.begin(); iter != terminals.end(); iter++)
	{
		if (!strcmp((*iter)->getValue(), "FROM"))
		{
			iter++;
			break;
		}
	}
	int count = 0;
	while (iter != terminals.end() && strcmp((*iter)->getValue(), "WHERE"))
	{
		if (strcmp((*iter)->getValue(), ","))
			count++;
		iter++;
	}
	if (count > 1)
		return false;
	return true;	
}

void merge(std::vector<OperandOperator*>& prefix, std::vector<OperandOperator*>& operand1, OperandOperator* operatorVal, std::vector<OperandOperator*>& operand2)
{
	std::vector<OperandOperator*>::iterator iter;
	prefix.push_back(operatorVal);
	for (iter = operand1.begin(); iter != operand1.end(); iter++)
		prefix.push_back((*iter));
	for (iter = operand2.begin(); iter != operand2.end(); iter++)
		prefix.push_back((*iter));
}

std::vector<OperandOperator*> getPrefixExpression(std::vector<parseTree*>& terminals, std::string& relationName, std::vector<parseTree*>::iterator& iter, bool& flag)
{
	std::vector<OperandOperator*> prefixExpression;
	char* initialVal = (*iter)->getValue();
	operandStack operandStackObj;
	operatorStack operatorStackObj;
	if (!strcmp(initialVal, "("))
	{
		OperandOperator* obj = new OperandOperator(std::string(initialVal), kOperandOperatorType::OPERATOR, "");
		operatorStackObj.push(obj);
	}
	else
	{
		OperandOperator* obj;
		if ((*iter)->getParent()->getNodeType() == TABLENAME)
		{
			iter += 2;
			char* attributeValue = (*iter)->getValue();
			obj = new OperandOperator(std::string(attributeValue), kOperandOperatorType::VARIABLE, std::string(initialVal));
		}
		else if((*iter)->getParent()->getNodeType() == ATTRIBUTENAME)
			obj = new OperandOperator(std::string(initialVal), kOperandOperatorType::VARIABLE, relationName);
		else if((*iter)->getParent()->getNodeType() == LITERAL || (*iter)->getParent()->getNodeType() == INTEGER)
			obj = new OperandOperator(std::string(initialVal), kOperandOperatorType::PATTERN, "");
		std::vector<OperandOperator*> v;
		v.push_back(obj);
		operandStackObj.push(v);
	}
	iter++;
	while (iter != terminals.end() && !operatorStackObj.isEmpty())
	{
		char* value = (*iter)->getValue();
		OperandOperator* obj;
		if ((!strcmp(value, "(")) || (!strcmp(value, "+")) || (!strcmp(value, "*")) || (!strcmp(value, "-")))
		{
			obj = new OperandOperator(std::string(value), kOperandOperatorType::OPERATOR, "");
			operatorStackObj.push(obj);
		}
		else if ((!strcmp(value, ")")))
		{
			OperandOperator* operatorVal = operatorStackObj.pop();
			while (strcmp(operatorVal->getName().c_str(), "("))
			{
				std::vector<OperandOperator*> operand1 = operandStackObj.pop();
				std::vector<OperandOperator*> operand2 = operandStackObj.pop();
				std::vector<OperandOperator*> prefix;
				merge(prefix, operand2, operatorVal, operand1);
				operandStackObj.push(prefix);
				operatorVal = operatorStackObj.pop();
			}
			delete operatorVal;
		}
		else
		{
			if ((*iter)->getParent()->getNodeType() == TABLENAME)
			{
				iter += 2;
				char* attributeValue = (*iter)->getValue();
				obj = new OperandOperator(std::string(attributeValue), kOperandOperatorType::VARIABLE, std::string(value));
			}
			else if ((*iter)->getParent()->getNodeType() == ATTRIBUTENAME)
				obj = new OperandOperator(std::string(value), kOperandOperatorType::VARIABLE, relationName);
			else if ((*iter)->getParent()->getNodeType() == LITERAL || (*iter)->getParent()->getNodeType() == INTEGER)
				obj = new OperandOperator(std::string(value), kOperandOperatorType::PATTERN, "");
			std::vector<OperandOperator*> v;
			v.push_back(obj);
			operandStackObj.push(v);
		}
		iter++;
	}
	std::vector<OperandOperator*> finalExpression = operandStackObj.pop();
	if (operatorStackObj.isEmpty())
		iter--;
	if (!(operandStackObj.isEmpty()))
		flag = false;
	else
		flag = true;
	return finalExpression;
}

std::vector<std::vector<JoinCondition*>> getJoinConditions(std::string& relationName, std::vector<parseTree*>& terminals, bool& flag)
{
	std::vector<std::vector<JoinCondition*>> joinConditions;
	vector<parseTree*>::iterator iter = terminals.begin();
	while (iter != terminals.end() && strcmp((*iter)->getValue(), "WHERE"))
	{
		iter++;
	}
	if (iter != terminals.end())
	{
		iter++;
		JoinCondition* currentJoinCondition;
		std::vector<JoinCondition*> currentAndConditionVector;
		std::vector<OperandOperator*> currentOperandExpression;
		while (iter != terminals.end() && strcmp((*iter)->getValue(), "ORDER BY"))
		{
			if ((!strcmp((*iter)->getValue(), "<")) || (!strcmp((*iter)->getValue(), ">")) || (!strcmp((*iter)->getValue(), "=")))
			{
				bool retVal = false;
				std::string operatorValueCurrent((*iter)->getValue());
				iter++;
				std::vector<OperandOperator*> operandExpression2 = getPrefixExpression(terminals, relationName, iter, retVal);
				if (!retVal)
				{
					flag = false;
					return joinConditions;
				}
				std::vector<OperandOperator*> temp = currentOperandExpression;
				currentJoinCondition = new JoinCondition(temp, operatorValueCurrent, operandExpression2);
				currentOperandExpression.clear();
			}
			else if ((!strcmp((*iter)->getValue(), "AND")))
			{
				currentAndConditionVector.push_back(currentJoinCondition);
				currentJoinCondition = NULL;
			}
			else if ((!strcmp((*iter)->getValue(), "OR")))
			{
				currentAndConditionVector.push_back(currentJoinCondition);
				currentJoinCondition = NULL;
				std::vector<JoinCondition*> temp = currentAndConditionVector;
				joinConditions.push_back(temp);
				currentAndConditionVector.clear();
			}
			else
			{
				bool retVal = false;
				currentOperandExpression = getPrefixExpression(terminals, relationName, iter, retVal);
				if (!retVal)
				{
					flag = false;
					return joinConditions;
				}
			}
			iter++;
		}
		currentAndConditionVector.push_back(currentJoinCondition);
		joinConditions.push_back(currentAndConditionVector);
	}
	flag = true;
	return joinConditions;
}

vector<OperandOperator*> getProjectionList(std::string relationName, std::vector<parseTree*>& terminals)
{
	vector<OperandOperator*> projectionAttributes;
	vector<parseTree*>::iterator iter = terminals.begin();
	iter++;
	if (!strcmp((*iter)->getValue(), "*"))
		return projectionAttributes;
	while (iter != terminals.end() && strcmp((*iter)->getValue(), "FROM"))
	{
		OperandOperator* obj;
		char* value = (*iter)->getValue();
		if ((*iter)->getParent()->getNodeType() == TABLENAME)
		{
			iter += 2;
			char* attributeValue = (*iter)->getValue();
			obj = new OperandOperator(std::string(attributeValue), kOperandOperatorType::VARIABLE, std::string(value));
			projectionAttributes.push_back(obj);
		}
		else if ((*iter)->getParent()->getNodeType() == ATTRIBUTENAME)
		{
			obj = new OperandOperator(std::string(value), kOperandOperatorType::VARIABLE, relationName);
			projectionAttributes.push_back(obj);
		}		
		iter++;
	}
	return projectionAttributes;
}

std::vector<OperandOperator*> getOrderAttributesList(std::string relationName, std::vector<parseTree*>& terminals)
{
	vector<OperandOperator*> orderAttributes;
	vector<parseTree*>::iterator iter = terminals.begin();
	while (iter != terminals.end() && strcmp((*iter)->getValue(), "ORDER BY"))
	{
		iter++;
	}
	while (iter != terminals.end())
	{
		OperandOperator* obj;
		char* value = (*iter)->getValue();
		if ((*iter)->getParent()->getNodeType() == TABLENAME)
		{
			iter += 2;
			char* attributeValue = (*iter)->getValue();
			obj = new OperandOperator(std::string(attributeValue), kOperandOperatorType::VARIABLE, std::string(value));
			orderAttributes.push_back(obj);
		}
		else if ((*iter)->getParent()->getNodeType() == ATTRIBUTENAME)
		{
			obj = new OperandOperator(std::string(value), kOperandOperatorType::VARIABLE, relationName);
			orderAttributes.push_back(obj);
		}
		iter++;
	}
	return orderAttributes;
}


std::vector<OperandOperator*> mergeVectors(std::vector<OperandOperator*>& projectionList, std::vector<OperandOperator*>& orderAttributes)
{
	std::vector<OperandOperator*> finalList = projectionList;
	for (std::vector<OperandOperator*>::iterator iter = orderAttributes.begin(); iter != orderAttributes.end(); iter++)
	{
		bool present = false;
		for (std::vector<OperandOperator*>::iterator iter2 = projectionList.begin(); iter2 != projectionList.end(); iter2++)
		{
			if (!strcmp((*iter)->getName().c_str(), (*iter2)->getName().c_str()) && !strcmp((*iter)->getTableName().c_str(), (*iter2)->getTableName().c_str()))
				present = true;
		}
		if (!present)
			finalList.push_back(*iter);
	}
	return finalList;
}

std::vector<std::string> getAttributeNames(std::vector<OperandOperator*>& attributesOperands)
{
	std::vector<std::string> attributeNames;
	for (std::vector<OperandOperator*>::iterator iter = attributesOperands.begin(); iter != attributesOperands.end(); iter++)
		attributeNames.push_back((*iter)->getName());
	return attributeNames;
}

bool distictStatementPresent(std::string relationName, std::vector<parseTree*>& terminals)
{
	for (std::vector<parseTree*>::iterator iter = terminals.begin(); iter != terminals.end(); iter++)
	{
		if (!strcmp((*iter)->getValue(), "DISTINCT"))
			return true;
	}
	return false;
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

	else if (pTree->getNodeType() == SELECTSTATEMENT)
	{
		if (singleTableStatement(terminals))
		{
			std::string relation_name = getRelationName(terminals);
			bool checkConditions = false;
			std::vector<std::vector<JoinCondition*>> joinConditionList = getJoinConditions(relation_name, terminals, checkConditions);
			Relation* selectWhereRelation;
			if (checkConditions)
			{
				Relation* intermediateRelation;
				vector<OperandOperator*> projectionList = getProjectionList(relation_name, terminals);
				std::vector<OperandOperator*> orderAttributes = getOrderAttributesList(relation_name, terminals);
				std::vector<OperandOperator*> finalProjectionAttributes = mergeVectors(projectionList, orderAttributes);
				selectWhereRelation = selectTable(relation_name, schemaManagerObj, joinConditionList, mainMemoryObj, finalProjectionAttributes);
				if (!selectWhereRelation)
				{
					fWriteExecute << "Execute of select failed\n\n\n\n\n";
					return;
				}
				intermediateRelation = selectWhereRelation;
				std::vector<std::string> projectionListString;
				if (finalProjectionAttributes.empty())
				{
					projectionListString = selectWhereRelation->getSchema().getFieldNames();
				}
				else
				{
					projectionListString = getAttributeNames(finalProjectionAttributes);
				}

				if (distictStatementPresent(relation_name, terminals))
				{
					bool successSort = false;
					vector<Relation*> sortedSubLists = sortSubList(selectWhereRelation, schemaManagerObj, NUM_OF_BLOCKS_IN_MEMORY - 1, finalProjectionAttributes, mainMemoryObj, successSort);
					for (int i = 0; i < sortedSubLists.size(); i++)
					{
						cout << "\n\n\n\n\n";
						sortedSubLists[i]->printRelation();
					}
					if (!successSort)
					{
						fWriteExecute << "Execute of select failed\n\n\n\n\n";
						return;
					}
					Relation* distictRelation;
					distictRelation = removeDuplicatesOperation(sortedSubLists, schemaManagerObj, mainMemoryObj, projectionListString);
					if (!distictRelation)
					{
						fWriteExecute << "Execute of select distict failed\n\n\n\n\n";
						return;
					}
					intermediateRelation = distictRelation;
				}

				if (!(orderAttributes.empty()))
				{
					bool success = false;
					vector<Relation*> sortedSubLists = sortSubList(intermediateRelation, schemaManagerObj, NUM_OF_BLOCKS_IN_MEMORY - 1, orderAttributes, mainMemoryObj, success);
					if (!success)
					{
						fWriteExecute << "Execute of select failed\n\n\n\n\n";
						return;
					}
					
					std::vector<std::string> orderAttributeString = getAttributeNames(orderAttributes);
					Relation* orderedRelation = sortOperation(sortedSubLists, schemaManagerObj, mainMemoryObj, orderAttributeString);
					if (!orderedRelation)
					{
						fWriteExecute << "Execute of select failed\n\n\n\n\n";
						return;
					}				
					intermediateRelation = orderedRelation;
				}
				joinConditionList.clear();
				Relation* finalRelation = selectTable(intermediateRelation->getRelationName(), schemaManagerObj, joinConditionList, mainMemoryObj, projectionList);
				if (!finalRelation)
				{
					fWriteExecute << "Execute of select failed\n\n\n\n\n";
					return;
				}
				fWriteExecute << "Execute of select succeeded\n\n\n\n\n";
				cout << "\n\n\n\n\n\n";
				finalRelation->printRelation();
				cout << "\n\n\n\n\n";
			}
		}
	}

	else if (pTree->getNodeType() == DELETESTATEMENT)
	{
		if (singleTableStatement(terminals))
		{
			std::string relation_name = getRelationName(terminals);
			bool checkConditions = false;
			std::vector<std::vector<JoinCondition*>> joinConditionList = getJoinConditions(relation_name, terminals, checkConditions);
			if (checkConditions)
			{
				bool retVal = deleteTable(relation_name, schemaManagerObj, joinConditionList, mainMemoryObj);
				if (!retVal)
					fWriteExecute << "Execute of delete failed\n\n\n\n\n";
				else
					fWriteExecute << "Delete succeeded\n\n\n\n\n";
			}
			else
				fWriteExecute << "Execute of delete failed\n\n\n\n\n";
		}
	}
}