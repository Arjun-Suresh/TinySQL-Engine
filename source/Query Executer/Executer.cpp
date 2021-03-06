#include "Executer.h"

std::string getSingleRelationName(std::vector<parseTree*>& terminals)
{
	for (std::vector<parseTree*>::iterator iter = terminals.begin(); iter != terminals.end(); iter++)
	{
		if ((*iter)->getParent()->getNodeType() == TABLENAME)
			return (*iter)->getValue();
	}
}

bool checkScopeOfAttributes(vector<vector<JoinCondition*>>& selectConditions, vector<OperandOperator*>& orderAttributes, vector<OperandOperator*>& projectionAttributes, vector<string>& tableNames)
{
	for (vector<vector<JoinCondition*>>::iterator iter = selectConditions.begin(); iter != selectConditions.end(); iter++)
	{
		vector<JoinCondition*> andConditions;
		for (vector<JoinCondition*>::iterator iteriter = (*iter).begin(); iteriter != (*iter).end(); iteriter++)
		{
			vector<OperandOperator*> operand1 = (*iteriter)->getOperand1();
			vector<OperandOperator*> operand2 = (*iteriter)->getOperand2();
			for (vector<OperandOperator*>::iterator opIter = operand1.begin(); opIter != operand1.end(); opIter++)
			{
				if ((*opIter)->getType() == kOperandOperatorType::VARIABLE)
				{
					string tableName = (*opIter)->getTableName();
					bool found = false;
					for (vector<string>::iterator tIter = tableNames.begin(); tIter != tableNames.end(); tIter++)
					{
						if (!strcmp(tableName.c_str(), (*tIter).c_str()))
						{
							found = true;
							break;
						}
					}
					if (!found)
						return false;
				}
			}

			for (vector<OperandOperator*>::iterator opIter = operand2.begin(); opIter != operand2.end(); opIter++)
			{
				if ((*opIter)->getType() == kOperandOperatorType::VARIABLE)
				{
					string tableName = (*opIter)->getTableName();
					bool found = false;
					for (vector<string>::iterator tIter = tableNames.begin(); tIter != tableNames.end(); tIter++)
					{
						if (!strcmp(tableName.c_str(), (*tIter).c_str()))
						{
							found = true;
							break;
						}
					}
					if (!found)
						return false;
				}
			}
		}
	}

	for (vector<OperandOperator*>::iterator iter = projectionAttributes.begin(); iter != projectionAttributes.end(); iter++)
	{
		string tableName = (*iter)->getTableName();
		bool found = false;
		for (vector<string>::iterator tIter = tableNames.begin(); tIter != tableNames.end(); tIter++)
		{
			if (!strcmp(tableName.c_str(), (*tIter).c_str()))
			{
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}

	for (vector<OperandOperator*>::iterator iter = orderAttributes.begin(); iter != orderAttributes.end(); iter++)
	{
		string tableName = (*iter)->getTableName();
		bool found = false;
		for (vector<string>::iterator tIter = tableNames.begin(); tIter != tableNames.end(); tIter++)
		{
			if (!strcmp(tableName.c_str(), (*tIter).c_str()))
			{
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;
}

bool doesFitInMemory(Relation* relation, int maxMemorySize)
{
	if (relation->getNumOfBlocks() <= maxMemorySize)
		return true;
	return false;
}

int min(int a, int b)
{
	return (a < b) ? (a) : (b);
}

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

std::vector<std::string> getFieldNames(std::vector<parseTree*>& terminals, int nodeType=0)
{
	std::vector<std::string> fieldNames;
	for (vector<parseTree*>::iterator iter = terminals.begin(); iter != terminals.end(); iter++)
	{
		if (nodeType && !strcmp((*iter)->getValue(), "SELECT"))
			break;
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
		else if ((*iter)->getParent()->getNodeType()==VALUE)
			values.push_back((*iter)->getValue());
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

std::vector<std::map<std::string, std::string>> getMapOfAttributes(Relation* selectStatementOutput, MainMemory& mainMemoryObj, std::vector<std::string>& originalFieldNames, bool& success)
{
	std::vector<std::map<std::string, std::string>> tuplesList;
	std::vector<std::string> valueList;
	vector<string> fieldNames = selectStatementOutput->getSchema().getFieldNames();
	for (int blockCount = 0; blockCount < selectStatementOutput->getNumOfBlocks(); blockCount++)
	{
		Block* blockPtr = mainMemoryObj.getBlock(0);
		blockPtr->clear();
		selectStatementOutput->getBlock(blockCount, 0);
		for (int tuplecount = 0; tuplecount < blockPtr->getNumTuples(); tuplecount++)
		{
			std::map<std::string, std::string> mapOfAttributes;
			Tuple output = blockPtr->getTuple(tuplecount);
			for (vector<string>::iterator iter = fieldNames.begin(); iter != fieldNames.end(); iter++)
			{
				Field value = output.getField(*iter);
				std::string valueString;
				if (output.getSchema().getFieldType(*iter) == INT)
					valueString = std::to_string(value.integer);
				else
					valueString = *(value.str);
				valueList.push_back(valueString);
			}
			if (originalFieldNames.size() != valueList.size())
			{
				success = false;
				return tuplesList;
			}
			for (int count = 0; count < originalFieldNames.size(); count++)
			{
				mapOfAttributes.insert(std::make_pair(originalFieldNames[count], valueList[count]));
			}
			valueList.clear();
			tuplesList.push_back(mapOfAttributes);
		}		
	}
	success = true;
	return tuplesList;
}

parseTree* isSimpleInsert(std::vector<parseTree*>& terminals)
{
	for (vector<parseTree*>::iterator iter = terminals.begin(); iter != terminals.end(); iter++)
	{
		if ((*iter)->getParent()->getNodeType() == SELECTSTATEMENT)
			return (*iter)->getParent();
	}
	return NULL;
}

bool getRelationNames(std::vector<parseTree*>& terminals, std::vector<std::string>& relations)
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
	while (iter != terminals.end() && strcmp((*iter)->getValue(), "WHERE") && strcmp((*iter)->getValue(), "ORDER BY"))
	{
		if (strcmp((*iter)->getValue(), ","))
			relations.push_back((*iter)->getValue());
		iter++;
	}
	if (relations.size() > 1)
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

std::vector<std::vector<JoinCondition*>> getSelectionConditions(std::string& relationName, std::vector<parseTree*>& terminals, bool& flag)
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

std::vector<std::vector<JoinCondition*>> getRelevantConditions(vector<vector<JoinCondition*>>& selectConditions, string relationName)
{
	vector<vector<JoinCondition*>> relevantConditions;
	for (vector<vector<JoinCondition*>>::iterator iter = selectConditions.begin(); iter != selectConditions.end(); iter++)
	{
		vector<JoinCondition*> andConditions;
		for (vector<JoinCondition*>::iterator iteriter = (*iter).begin(); iteriter != (*iter).end(); iteriter++)
		{
			bool flag1 = true, flag2 = true;
			vector<OperandOperator*> operand1 = (*iteriter)->getOperand1();
			vector<OperandOperator*> operand2 = (*iteriter)->getOperand2();
			for (vector<OperandOperator*>::iterator opIter = operand1.begin(); opIter != operand1.end(); opIter++)
			{
				if ((*opIter)->getType() == kOperandOperatorType::VARIABLE && strcmp((*opIter)->getTableName().c_str(), relationName.c_str()))
				{
					flag1 = false;
					break;
				}
			}
			for (vector<OperandOperator*>::iterator opIter = operand2.begin(); opIter != operand2.end(); opIter++)
			{
				if ((*opIter)->getType() == kOperandOperatorType::VARIABLE && strcmp((*opIter)->getTableName().c_str(), relationName.c_str()))
				{
					flag2 = false;
					break;
				}
			}
			if (flag1 && flag2)
				andConditions.push_back(*iteriter);
		}
		if (!andConditions.empty())
			relevantConditions.push_back(andConditions);
	}
	return relevantConditions;
}


std::vector<std::vector<JoinCondition*>> getRelevantJoinConditions(vector<vector<JoinCondition*>>& selectConditions, vector<string>& relationNames1, string relationName2)
{
	vector<vector<JoinCondition*>> relevantJoinConditions;
	for (vector<vector<JoinCondition*>>::iterator iter = selectConditions.begin(); iter != selectConditions.end(); iter++)
	{
		vector<JoinCondition*> andConditions;
		for (vector<JoinCondition*>::iterator iteriter = (*iter).begin(); iteriter != (*iter).end(); iteriter++)
		{
			bool relationFound1=false,relationFound2 = false;
			vector<OperandOperator*> operand1 = (*iteriter)->getOperand1();
			vector<OperandOperator*> operand2 = (*iteriter)->getOperand2();
			OperandOperator* val;
			for (vector<OperandOperator*>::iterator opIter = operand1.begin(); opIter != operand1.end(); opIter++)
			{
				if ((*opIter)->getType() == kOperandOperatorType::VARIABLE)
				{
					if(!strcmp((*opIter)->getTableName().c_str(), relationName2.c_str()))
						relationFound2 = true;
					else
					{
						if (!relationFound1)
						{
							for (vector<string>::iterator relIter = relationNames1.begin(); relIter != relationNames1.end(); relIter++)
							{
								if (!strcmp((*opIter)->getTableName().c_str(), (*relIter).c_str()))
								{
									relationFound1 = true;
									break;
								}
							}
						}
					}
				}
			}
			for (vector<OperandOperator*>::iterator opIter = operand2.begin(); opIter != operand2.end(); opIter++)
			{
				if ((*opIter)->getType() == kOperandOperatorType::VARIABLE)
				{
					if (!strcmp((*opIter)->getTableName().c_str(), relationName2.c_str()))
						relationFound2 = true;
					else
					{
						if (!relationFound1)
						{
							for (vector<string>::iterator relIter = relationNames1.begin(); relIter != relationNames1.end(); relIter++)
							{
								if (!strcmp((*opIter)->getTableName().c_str(), (*relIter).c_str()))
								{
									relationFound1 = true;
									break;
								}
							}
						}
					}
				}
			}
			if (relationFound1 && relationFound2)
				andConditions.push_back(*iteriter);
		}
		if (!andConditions.empty())
			relevantJoinConditions.push_back(andConditions);
	}
	return relevantJoinConditions;
}

bool distictStatementPresent(std::vector<parseTree*>& terminals)
{
	for (std::vector<parseTree*>::iterator iter = terminals.begin(); iter != terminals.end(); iter++)
	{
		if (!strcmp((*iter)->getValue(), "DISTINCT"))
			return true;
	}
	return false;
}

void renameAttributes(vector<OperandOperator*>& attributeList)
{
	for (vector<OperandOperator*>::iterator iter = attributeList.begin(); iter != attributeList.end(); iter++)
		(*iter)->setName((*iter)->getTableName() + "." + (*iter)->getName());
}

void renameConditions(vector<vector<JoinCondition*>>& conditions)
{
	for (vector<vector<JoinCondition*>>::iterator iter = conditions.begin(); iter != conditions.end(); iter++)
	{
		vector<JoinCondition*> andConditions;
		for (vector<JoinCondition*>::iterator iteriter = (*iter).begin(); iteriter != (*iter).end(); iteriter++)
		{
			bool relationFound1 = false, relationFound2 = false;;
			vector<OperandOperator*> operand1 = (*iteriter)->getOperand1();
			vector<OperandOperator*> operand2 = (*iteriter)->getOperand2();
			OperandOperator* val;
			for (vector<OperandOperator*>::iterator opIter = operand1.begin(); opIter != operand1.end(); opIter++)
			{
				if ((*opIter)->getType() == kOperandOperatorType::VARIABLE)
					(*opIter)->setName((*opIter)->getTableName() + "." + (*opIter)->getName());
			}
			for (vector<OperandOperator*>::iterator opIter = operand2.begin(); opIter != operand2.end(); opIter++)
			{
				if ((*opIter)->getType() == kOperandOperatorType::VARIABLE)
					(*opIter)->setName((*opIter)->getTableName() + "." + (*opIter)->getName());
			}
		}
	}
}

bool executeQuery(SchemaManager& schemaManagerObj, MainMemory& mainMemoryObj, Disk& diskObj, statement* statementObj, std::ofstream& fWriteExecute, vector<Relation*>& selectStmtOutput, bool returnOutPut)
{
	parseTree* pTree = statementObj->getRoot();
	return executeQuery(schemaManagerObj, mainMemoryObj, diskObj, pTree, fWriteExecute, selectStmtOutput, returnOutPut);
}

void splitJoinConditions(vector<JoinCondition*>& curJoinConditions, vector<JoinCondition*>& eqiJoinConditions, vector<JoinCondition*>& otherConditions, vector<string>& tableNames1, string tableName2)
{
	for (vector<JoinCondition*>::iterator iter = curJoinConditions.begin(); iter != curJoinConditions.end(); iter++)
	{
		bool relation1FoundOperand1 = false, relation2FoundOperand1 = false;
		vector<OperandOperator*> operand1 = (*iter)->getOperand1();
		vector<OperandOperator*> operand2 = (*iter)->getOperand2();
		for (vector<OperandOperator*>::iterator opIter = operand1.begin(); opIter != operand1.end(); opIter++)
		{
			if ((*opIter)->getType() == kOperandOperatorType::VARIABLE)
			{
				if (!strcmp((*opIter)->getTableName().c_str(), tableName2.c_str()))
					relation2FoundOperand1 = true;
				else
				{
					if (!relation1FoundOperand1)
					{
						for (vector<string>::iterator relIter = tableNames1.begin(); relIter != tableNames1.end(); relIter++)
						{
							if (!strcmp((*opIter)->getTableName().c_str(), (*relIter).c_str()))
							{
								relation1FoundOperand1 = true;
								break;
							}
						}
					}
				}
			}
		}
		if (relation1FoundOperand1 && relation2FoundOperand1)
			continue;
		bool relation1FoundOperand2 = false, relation2FoundOperand2 = false;
		for (vector<OperandOperator*>::iterator opIter = operand2.begin(); opIter != operand2.end(); opIter++)
		{
			if ((*opIter)->getType() == kOperandOperatorType::VARIABLE)
			{
				if (!strcmp((*opIter)->getTableName().c_str(), tableName2.c_str()))
					relation2FoundOperand2 = true;
				else
				{
					if (!relation2FoundOperand1)
					{
						for (vector<string>::iterator relIter = tableNames1.begin(); relIter != tableNames1.end(); relIter++)
						{
							if (!strcmp((*opIter)->getTableName().c_str(), (*relIter).c_str()))
							{
								relation2FoundOperand1 = true;
								break;
							}
						}
					}
				}
			}
		}

		if ((relation1FoundOperand1 != relation1FoundOperand2) && (relation2FoundOperand1 != relation2FoundOperand2))
		{
			if (!strcmp((*iter)->getOperatorOfOperation().c_str(), "="))
			{
				if (relation2FoundOperand1)
				{
					(*iter)->setOperand1(operand2);
					(*iter)->setOperand2(operand1);
				}
				eqiJoinConditions.push_back(*iter);
			}
			else
				otherConditions.push_back(*iter);
		}
	}
}

void getJoinAttributes(std::vector<JoinCondition*>& eqiJoinConditions, std::vector<OperandOperator*>& joinAttributesRel1, std::vector<OperandOperator*>& joinAttributesRel2)
{
	for (vector<JoinCondition*>::iterator iter = eqiJoinConditions.begin(); iter != eqiJoinConditions.end(); iter++)
	{
		vector<OperandOperator*> operand1 = (*iter)->getOperand1();
		vector<OperandOperator*> operand2 = (*iter)->getOperand2();
		for (vector<OperandOperator*>::iterator opIter = operand1.begin(); opIter != operand1.end(); opIter++)
		{
			if ((*opIter)->getType() == kOperandOperatorType::VARIABLE)
				joinAttributesRel1.push_back(*opIter);
		}
		for (vector<OperandOperator*>::iterator opIter = operand2.begin(); opIter != operand2.end(); opIter++)
		{
			if ((*opIter)->getType() == kOperandOperatorType::VARIABLE)
				joinAttributesRel2.push_back(*opIter);
		}
	}
}

bool executeQuery(SchemaManager& schemaManagerObj, MainMemory& mainMemoryObj, Disk& diskObj, parseTree* pTree, std::ofstream& fWriteExecute, vector<Relation*>& selectStmtOutput, bool returnOutPut)
{
	std::vector<parseTree*> terminals;
	getTerminals(pTree, terminals);
	if (pTree->getNodeType() == CREATETABLESTATEMENT)
	{
		std::string relation_name = getSingleRelationName(terminals);
		std::vector<std::string> field_names = getFieldNames(terminals);
		std::vector<enum FIELD_TYPE> field_types = getFieldTypes(terminals);
		Relation * relationObj = createTable(schemaManagerObj, relation_name, field_names, field_types, fWriteExecute);
		if (!relationObj)
		{
			fWriteExecute << "Execute of create failed\n\n";
			return false;
		}
		fWriteExecute << "Create was successful\n\n";
		return true;
	}

	else if (pTree->getNodeType() == DROPTABLESTATEMENT)
	{
		std::string relation_name = getSingleRelationName(terminals);
		bool retVal = dropTable(relation_name, fWriteExecute, schemaManagerObj);
		if (!retVal)
		{
			fWriteExecute << "Execute of drop failed\n\n";
			return false;
		}
		fWriteExecute << "Drop succeeded\n\n";
		return true;
	}

	else if (pTree->getNodeType() == INSERTSTATEMENT)
	{
		std::string relation_name = getSingleRelationName(terminals);
		bool success = false;
		std::map<std::string, std::string> mapOfAttributes;
		parseTree* selectStmtNode = isSimpleInsert(terminals);
		bool retVal;
		if (!selectStmtNode)
		{
			mapOfAttributes = getMapOfAttributes(terminals, success);
			if (!success)
			{
				fWriteExecute << "The column list and field values don't match. Execute of insert failed\n\n";
				return false;
			}
			retVal = insertTable(relation_name, schemaManagerObj, mapOfAttributes, mainMemoryObj, fWriteExecute);
		}
		else
		{
			vector<Relation*> selectStatementOutput;
			executeQuery(schemaManagerObj, mainMemoryObj, diskObj, selectStmtNode, fWriteExecute, selectStatementOutput, true);
			if (selectStatementOutput.size() == 0)
			{
				fWriteExecute << "Select didn't return any tuple. Execute of insert failed\n\n";
				return false;
			}
			
			vector<string> fieldNames = getFieldNames(terminals, SELECTSTATEMENT);
			std::vector<std::map<std::string, std::string>> tupleList;
			tupleList = getMapOfAttributes(selectStatementOutput[0], mainMemoryObj, fieldNames, success);
			if (!success)
			{
				fWriteExecute << "The column list and field values don't match. Execute of insert failed\n\n\n\n\n";
				return false;
			}
			for (int i = 0; i < tupleList.size(); i++)
			{				
				retVal = insertTable(relation_name, schemaManagerObj, tupleList[i], mainMemoryObj, fWriteExecute);
			}
		}
		deleteAllIntermediateTables(fWriteExecute, schemaManagerObj);
		if (!retVal)
		{
			fWriteExecute << "Execute of insert failed\n\n";
			return false;
		}
		fWriteExecute << "Insert succeeded\n\nDump of the relation:\n";
		schemaManagerObj.getRelation(relation_name)->printRelation(fWriteExecute);
		return true;
	}

	else if (pTree->getNodeType() == SELECTSTATEMENT)
	{
		std::vector<std::string> tableNames;
		if (getRelationNames(terminals, tableNames))
		{
			std::string relation_name = tableNames[0];
			bool checkConditions = false;
			std::vector<std::vector<JoinCondition*>> joinConditionList = getSelectionConditions(relation_name, terminals, checkConditions);
			vector<OperandOperator*> projectionList = getProjectionList(relation_name, terminals);
			std::vector<OperandOperator*> orderAttributes = getOrderAttributesList(relation_name, terminals);
			Relation* selectWhereRelation;
			if (!checkScopeOfAttributes(joinConditionList, orderAttributes, projectionList, tableNames))
			{
				fWriteExecute << "Some of the attributes specified don't belong to the scope of the tables specified in the query. Execute of select failed\n\n";
				return false;
			}
			if (checkConditions)
			{
				Relation* intermediateRelation;
				std::vector<OperandOperator*> finalProjectionAttributes;
				if(!projectionList.empty())
					finalProjectionAttributes = mergeVectors(projectionList, orderAttributes);
				selectWhereRelation = selectTable(relation_name, schemaManagerObj, joinConditionList, mainMemoryObj, finalProjectionAttributes, false, fWriteExecute);
				if (!selectWhereRelation)
				{
					fWriteExecute << "Execute of select failed\n\n";
					return false;
				}
				intermediateRelation = selectWhereRelation;
				std::vector<std::string> projectionListString;
				if (finalProjectionAttributes.empty())
				{
					projectionListString = selectWhereRelation->getSchema().getFieldNames();
					for (vector<string>::iterator iter = projectionListString.begin(); iter != projectionListString.end(); iter++)
					{
						OperandOperator* obj = new OperandOperator((*iter), kOperandOperatorType::VARIABLE, "");
						finalProjectionAttributes.push_back(obj);
					}
				}
				else
				{
					projectionListString = getAttributeNames(finalProjectionAttributes);
				}

				if (distictStatementPresent(terminals))
				{
					if (doesFitInMemory(selectWhereRelation, NUM_OF_BLOCKS_IN_MEMORY - 1))
					{
						fWriteExecute << "Executing one-pass select distinct\n\n";
						intermediateRelation = onePassDistinct(selectWhereRelation, mainMemoryObj, schemaManagerObj, projectionList, fWriteExecute);
						if (!intermediateRelation)
						{
							fWriteExecute << "Execute of one-pass select distict failed\n\n";
							return false;
						}

					}

					else
					{
						bool successSort = false;
						vector<Relation*> sortedSubLists = sortSubList(selectWhereRelation, schemaManagerObj, NUM_OF_BLOCKS_IN_MEMORY - 1, finalProjectionAttributes, mainMemoryObj, successSort, fWriteExecute);
						if (!successSort)
						{
							fWriteExecute << "Creation of ordered sublists for distinct failed. Execute of select failed\n\n";
							return false;
						}
						Relation* distictRelation = NULL;
						int countOfSubLists = 0;
						vector<Relation*> subListsSet;
						while (countOfSubLists < sortedSubLists.size())
						{
							int i;
							for (i = countOfSubLists; i < min(sortedSubLists.size(), countOfSubLists + (NUM_OF_BLOCKS_IN_MEMORY - 1)); i++)
							{
								subListsSet.push_back(sortedSubLists[i]);
							}
							distictRelation = removeDuplicatesOperation(subListsSet, schemaManagerObj, mainMemoryObj, projectionListString);
							if (!distictRelation)
							{
								fWriteExecute << "Execute of select distict failed\n\n";
								return false;
							}
							subListsSet.clear();
							subListsSet.push_back(distictRelation);
							countOfSubLists += i;
						}
						intermediateRelation = distictRelation;
					}
				}
				Relation* finalRelation = NULL;
				if (!(orderAttributes.empty()))
				{
					if (doesFitInMemory(intermediateRelation, NUM_OF_BLOCKS_IN_MEMORY - 1))
					{
						fWriteExecute << "Executing one-pass select order by\n\n";
						Relation* orderedRelation = onePassOrdering(intermediateRelation, mainMemoryObj, schemaManagerObj, orderAttributes, fWriteExecute);
						if (!orderedRelation)
						{
							fWriteExecute << "Execute of one-pass order by failed\n\n";
							return false;
						}
						intermediateRelation = orderedRelation;
					}
					else
					{
						bool success = false;
						vector<Relation*> sortedSubLists = sortSubList(intermediateRelation, schemaManagerObj, NUM_OF_BLOCKS_IN_MEMORY - 1, orderAttributes, mainMemoryObj, success, fWriteExecute);
						if (!success)
						{
							fWriteExecute << "Creation of ordered sublists for order by failed. Execute of select failed\n\n";
							return false;
						}

						std::vector<std::string> orderAttributeString = getAttributeNames(orderAttributes);
						Relation* orderedRelation = NULL;
						int countOfSubLists = 0;
						vector<Relation*> subListsSet;
						while (countOfSubLists < sortedSubLists.size())
						{
							int i;
							for (i = countOfSubLists; i < min(sortedSubLists.size(), countOfSubLists + (NUM_OF_BLOCKS_IN_MEMORY - 1)); i++)
							{
								subListsSet.push_back(sortedSubLists[i]);
							}
							orderedRelation = sortOperation(sortedSubLists, schemaManagerObj, mainMemoryObj, orderAttributeString);
							if (!orderedRelation)
							{
								fWriteExecute << "Execute of order by failed\n\n";
								return false;
							}
							subListsSet.clear();
							subListsSet.push_back(orderedRelation);
							countOfSubLists += i;
						}
						intermediateRelation = orderedRelation;
						joinConditionList.clear();
						finalRelation = selectTable(intermediateRelation->getRelationName(), schemaManagerObj, joinConditionList, mainMemoryObj, projectionList, false, fWriteExecute);
					}
				}
				else
					finalRelation = intermediateRelation;
				if (!finalRelation)
				{
					fWriteExecute << "Execute of select failed\n\n";
					return false;
				}
				if (!returnOutPut)
				{
					fWriteExecute << "Execute of select succeeded\n\n. Dump of the output:\n";
					finalRelation->printRelation(fWriteExecute);
					fWriteExecute << "\n\n";
					deleteAllIntermediateTables(fWriteExecute, schemaManagerObj);
				}
				else
					selectStmtOutput.push_back(finalRelation);
				return true;
			}
		}


		else
		{
			vector<Relation*> relationsToBeMerged;
			bool conditionListPopulated = false;
			std::vector<std::vector<JoinCondition*>> selectConditions = getSelectionConditions(string(""), terminals, conditionListPopulated);
			std::vector<OperandOperator*> actualProjectionList = getProjectionList(string(""), terminals);
			std::vector<OperandOperator*> actualOrderedList = getOrderAttributesList(string(""), terminals);
			if (!checkScopeOfAttributes(selectConditions, actualOrderedList, actualProjectionList, tableNames))
			{
				fWriteExecute << "Some of the attributes specified don't belong to the scope of the tables specified in the query. Execute of select failed\n\n";
				return false;
			}
			if (!conditionListPopulated)
			{
				fWriteExecute << "Error while parsing through the conditions. Execution of select failed\n\n";
				return false;
			}
			for (vector<string>::iterator relationIter = tableNames.begin(); relationIter != tableNames.end(); relationIter++)
			{
				std::vector<std::vector<JoinCondition*>> subsetConditions;
				if (selectConditions.size()<2)
					subsetConditions = getRelevantConditions(selectConditions, (*relationIter));
				std::vector<OperandOperator*> intermediateProjectionList;
				Relation* intermediateObj = selectTable((*relationIter), schemaManagerObj, subsetConditions, mainMemoryObj, intermediateProjectionList, true, fWriteExecute);
				if (intermediateObj->getNumOfTuples() == 0)
				{
					fWriteExecute << "Table " << (*relationIter) << " seems to be empty\n";
					return true;
				}
				relationsToBeMerged.push_back(intermediateObj);
			}
			renameAttributes(actualOrderedList);
			renameAttributes(actualProjectionList);
			renameConditions(selectConditions);
			vector<Relation*> currentTables;
			int tablesCoveredSoFar = 1;
			currentTables.push_back(relationsToBeMerged[0]);
			vector<string> tableNamesAlreadyJoined;
			tableNamesAlreadyJoined.push_back(tableNames[tablesCoveredSoFar - 1]);
			for (vector<Relation*>::iterator tableIter = relationsToBeMerged.begin() + 1; tableIter != relationsToBeMerged.end(); tableIter++)
			{
				Relation* joinedTable;
				vector<JoinCondition*> eqiJoinConditions, otherConditions;
				bool equiJoin = false;
				currentTables.push_back(*tableIter);
				vector<vector<JoinCondition*>> curJoinConditions;
				if (selectConditions.size() < 2)
				{
					curJoinConditions = getRelevantJoinConditions(selectConditions, tableNamesAlreadyJoined, tableNames[tablesCoveredSoFar]);
					if (!curJoinConditions.empty())
					{
						splitJoinConditions(curJoinConditions[0], eqiJoinConditions, otherConditions, tableNamesAlreadyJoined, tableNames[tablesCoveredSoFar]);
						if (!eqiJoinConditions.empty())
							equiJoin = true;
					}
				}						
				else
					curJoinConditions = selectConditions;
				if(!equiJoin)
					joinedTable = cartesianProductOnePass(currentTables, schemaManagerObj, mainMemoryObj, curJoinConditions, fWriteExecute);
				else
				{
					vector<OperandOperator*> joinAttributesRel1, joinAttributesRel2;
					bool successSort = false;
					getJoinAttributes(eqiJoinConditions, joinAttributesRel1, joinAttributesRel2);
					vector<Relation*> sortedSubListsTable1 = sortSubList(currentTables[0], schemaManagerObj, NUM_OF_BLOCKS_IN_MEMORY/2 - 1, joinAttributesRel1, mainMemoryObj, successSort, fWriteExecute);
					if (!successSort)
					{
						fWriteExecute << "Creating sorted sublists for joi failed. Execute of select failed\n\n\n\n\n";
						return false;
					}
					vector<Relation*> sortedSubListsTable2 = sortSubList(currentTables[1], schemaManagerObj, NUM_OF_BLOCKS_IN_MEMORY / 2 - 1, joinAttributesRel2, mainMemoryObj, successSort, fWriteExecute);
					
					if (!successSort)
					{
						fWriteExecute << "Creating sorted sublists for join failed. Execute of select failed\n\n\n\n\n";
						return false;
					}
					Relation* intermediateJoinedTable = createSchemaForJoinTable(sortedSubListsTable1, sortedSubListsTable2, schemaManagerObj);

					for (int i = 0; i < sortedSubListsTable1.size();)
					{
						vector<Relation*> sortedSubListsRelation1;
						int sublistNum;
						for (sublistNum = i; sublistNum < sortedSubListsTable1.size() && sublistNum < i + (NUM_OF_BLOCKS_IN_MEMORY / 2 - 1); sublistNum++)
							sortedSubListsRelation1.push_back(sortedSubListsTable1[sublistNum]);
						i = sublistNum;
						for (int j = 0; j < sortedSubListsTable2.size();)
						{
							vector<Relation*> sortedSubListsRelation2;
							int sublistNum2;
							for (sublistNum2 = j; sublistNum2 < sortedSubListsTable2.size() && sublistNum2 < j + (NUM_OF_BLOCKS_IN_MEMORY / 2 - 1); sublistNum2++)
								sortedSubListsRelation2.push_back(sortedSubListsTable2[sublistNum2]);
							j += sublistNum2;
							joinTables(sortedSubListsRelation1, sortedSubListsRelation2, schemaManagerObj, mainMemoryObj, eqiJoinConditions, intermediateJoinedTable);
						}
					}
					vector<vector<JoinCondition*>> otherSelectConditions;
					otherSelectConditions.push_back(otherConditions);
					vector<OperandOperator*> emptyProjectionList;
					if (!otherConditions.empty())
						joinedTable = selectTable(intermediateJoinedTable->getRelationName(), schemaManagerObj, otherSelectConditions, mainMemoryObj, emptyProjectionList, false, fWriteExecute);
					else
						joinedTable = intermediateJoinedTable;
				}
				currentTables.clear();
				currentTables.push_back(joinedTable);
				tablesCoveredSoFar++;
				tableNamesAlreadyJoined.push_back(tableNames[tablesCoveredSoFar - 1]);
			}
			Relation* joinedRelation = currentTables[0];
			vector<string> projectionListString;
			if (actualProjectionList.empty())
			{
				projectionListString = joinedRelation->getSchema().getFieldNames();
				for (vector<string>::iterator iter = projectionListString.begin(); iter != projectionListString.end(); iter++)
				{
					OperandOperator* obj = new OperandOperator((*iter), kOperandOperatorType::VARIABLE, "");
					actualProjectionList.push_back(obj);
				}
			}
			else
			{
				projectionListString = getAttributeNames(actualProjectionList);
			}
			if (distictStatementPresent(terminals))
			{
				if (doesFitInMemory(joinedRelation, NUM_OF_BLOCKS_IN_MEMORY - 1))
				{
					fWriteExecute << "Executing one-pass select distinct operation\n\n";
					Relation* distinctRelation = onePassDistinct(joinedRelation, mainMemoryObj, schemaManagerObj, actualProjectionList, fWriteExecute);
					if (!distinctRelation)
					{
						fWriteExecute << "Execute of one-pass select distict failed\n\n";
						return false;
					}
					joinedRelation = distinctRelation;
				}
				else
				{
					bool successSort = false;
					vector<Relation*> sortedSubLists = sortSubList(joinedRelation, schemaManagerObj, NUM_OF_BLOCKS_IN_MEMORY - 1, actualProjectionList, mainMemoryObj, successSort, fWriteExecute);
					if (!successSort)
					{
						fWriteExecute << "Creating sorted sublists for distinct failed. Execute of select failed\n\n\n\n\n";
						return false;
					}

					Relation* distictRelation = NULL;
					int countOfSubLists = 0;
					vector<Relation*> subListsSet;
					while (countOfSubLists < sortedSubLists.size())
					{
						int i;
						for (i = countOfSubLists; i < min(sortedSubLists.size(), countOfSubLists + (NUM_OF_BLOCKS_IN_MEMORY - 1)); i++)
						{
							subListsSet.push_back(sortedSubLists[i]);
						}
						distictRelation = removeDuplicatesOperation(subListsSet, schemaManagerObj, mainMemoryObj, projectionListString);
						if (!distictRelation)
						{
							fWriteExecute << "Execute of select distict failed\n\n";
							return false;
						}
						subListsSet.clear();
						subListsSet.push_back(distictRelation);
						countOfSubLists += i;
					}
					joinedRelation = distictRelation;
				}
			}

			if (!(actualOrderedList.empty()))
			{
				if (doesFitInMemory(joinedRelation, NUM_OF_BLOCKS_IN_MEMORY - 1))
				{
					fWriteExecute << "Executing one-pass order algorithm\n\n";
					Relation* orderedRelation = onePassOrdering(joinedRelation, mainMemoryObj, schemaManagerObj, actualOrderedList, fWriteExecute);
					if (!orderedRelation)
					{
						fWriteExecute << "Execute of one-pass order by failed\n\n";
						return false;
					}
					joinedRelation = orderedRelation;
				}
				else
				{
					bool success = false;
					vector<Relation*> sortedSubLists = sortSubList(joinedRelation, schemaManagerObj, NUM_OF_BLOCKS_IN_MEMORY - 1, actualOrderedList, mainMemoryObj, success, fWriteExecute);
					if (!success)
					{
						fWriteExecute << "Creation of sorted sublists for order by failed. Execute of select failed\n\n";
						return false;
					}

					std::vector<std::string> orderAttributeString = getAttributeNames(actualOrderedList);
					Relation* orderedRelation = NULL;
					int countOfSubLists = 0;
					vector<Relation*> subListsSet;
					while (countOfSubLists < sortedSubLists.size())
					{
						int i;
						for (i = countOfSubLists; i < min(sortedSubLists.size(), countOfSubLists + (NUM_OF_BLOCKS_IN_MEMORY - 1)); i++)
						{
							subListsSet.push_back(sortedSubLists[i]);
						}
						orderedRelation = sortOperation(sortedSubLists, schemaManagerObj, mainMemoryObj, orderAttributeString);
						if (!orderedRelation)
						{
							fWriteExecute << "Execute of order by failed\n\n";
							return false;
						}
						subListsSet.clear();
						subListsSet.push_back(orderedRelation);
						countOfSubLists += i;
					}
					joinedRelation = orderedRelation;
				}
			}
			selectConditions.clear();
			Relation* finalRelation = selectTable(joinedRelation->getRelationName(), schemaManagerObj, selectConditions, mainMemoryObj, actualProjectionList, false, fWriteExecute);
			if (!finalRelation)
			{
				fWriteExecute << "Execute of select failed\n\n";
				return false;
			}
			if (!returnOutPut)
			{
				fWriteExecute << "Execute of select succeeded.\n The relation output sump is:\n";
				finalRelation->printRelation(fWriteExecute);
				fWriteExecute << "\n\n";
				deleteAllIntermediateTables(fWriteExecute, schemaManagerObj);
			}
			else
				selectStmtOutput.push_back(finalRelation);
			return true;
		}
	}

	else if (pTree->getNodeType() == DELETESTATEMENT)
	{
		std::vector<std::string> tableNames;
		getRelationNames(terminals, tableNames);
		std::string relation_name = tableNames[0];
		bool checkConditions = false;
		std::vector<std::vector<JoinCondition*>> joinConditionList = getSelectionConditions(relation_name, terminals, checkConditions);
		if (checkConditions)
		{
			bool retVal = deleteTable(relation_name, schemaManagerObj, joinConditionList, mainMemoryObj, fWriteExecute);
			if (!retVal)
			{
				fWriteExecute << "Execute of delete failed\n\n";
				return false;
			}
			fWriteExecute << "Delete succeeded\n. Dump of the relation:\n";
			schemaManagerObj.getRelation(relation_name)->printRelation(fWriteExecute);
			fWriteExecute << "\n\n";
			return true;
		}
		else
		{
			fWriteExecute << "Error occurred while checking the conditions. Execute of delete failed\n\n";
			return false;
		}
	}
}