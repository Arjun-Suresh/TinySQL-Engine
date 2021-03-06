#include "Operations.h"


std::ofstream fout;

int tableIndex = 0;
std::string baseName = "Intermediate_table_";

string getIntermediateTableName()
{
	char relationNum[10];
	_itoa_s(tableIndex++, relationNum, 10);
	return baseName + relationNum;
}

void deleteAllIntermediateTables(std::ofstream& fileOutput, SchemaManager& schema)
{
	char relationNum[10];
	int tablesDeleted = 0;
	for (int i = 0; i < tableIndex; i++)
	{
		_itoa_s(i, relationNum, 10);
		string tableName = baseName + relationNum;
		if (schema.relationExists(tableName))
		{
			dropTable(tableName, fileOutput, schema);
			tablesDeleted++;
		}
	}
	schema.reduceOffset(tablesDeleted);
}

Relation * createTable(SchemaManager& schema_manager, string relation_name, vector<string> field_names, vector<enum FIELD_TYPE> field_types, std::ofstream& fwriteExecute) {
	Schema schema(field_names, field_types);
	Relation* relation_ptr = schema_manager.createRelation(relation_name, schema);
	if (relation_ptr == NULL) {
		fwriteExecute << "Table with name " << relation_name << " Has failed to be created\n\n";
	}
	return relation_ptr;
}

void appendTupleToRelation(Relation* relation_ptr, MainMemory& mem, int memory_block_index, Tuple& tuple) {
	Block* block_ptr;
	if (relation_ptr->getNumOfBlocks() == 0) {
		block_ptr = mem.getBlock(memory_block_index);
		block_ptr->clear();
		block_ptr->appendTuple(tuple);
		relation_ptr->setBlock(relation_ptr->getNumOfBlocks(), memory_block_index);
	}
	else {
		block_ptr = mem.getBlock(memory_block_index);
		block_ptr->clear();
		relation_ptr->getBlock(relation_ptr->getNumOfBlocks() - 1, memory_block_index);

		if (block_ptr->isFull()) {
			block_ptr->clear();
			block_ptr->appendTuple(tuple);
			relation_ptr->setBlock(relation_ptr->getNumOfBlocks(), memory_block_index);
		}
		else {
			block_ptr->appendTuple(tuple);

			relation_ptr->setBlock(relation_ptr->getNumOfBlocks() - 1, memory_block_index);
		}
	}
}


inline bool isInteger(const std::string & s)
{
	if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

	char * p;
	strtol(s.c_str(), &p, 10);

	return (*p == 0);
}



bool insertTable(string tableName, SchemaManager& schema_manager, std::map<string, string> map_of_attributes, MainMemory& mem, std::ofstream& fwriteExecute) {
	try {
		Relation * relation = schema_manager.getRelation(tableName);
		if (relation == NULL) {
			throw "Table" + tableName + " Does not exist";
		}
		Tuple t = relation->createTuple();
		Schema scheme_of_tuple = t.getSchema();
		bool result = false;
		map<string, string>::const_iterator map_iterator;
		for (map_iterator = map_of_attributes.cbegin(); map_iterator != map_of_attributes.cend(); map_iterator++) {
			string field_name = map_iterator->first;
			string field_value = map_iterator->second;
			FIELD_TYPE	 field_type = scheme_of_tuple.getFieldType(field_name);
			if (field_type == STR20)
			{
				result = t.setField(field_name, field_value);
				if (!result) {
					throw "Setting field " + field_name + " " + field_value + "Has failed";
				}
			}
			else {
				//Check if it is a integer
				result = isInteger(field_value);
				if (!result) {
					throw "Setting field " + field_name + " because " + field_value + " is not a integer";
				}
				result = t.setField(field_name, std::stoi(field_value));
				if (!result) {
					throw "Setting field " + field_name + " " + field_value + "Has failed";
				}

			}
		}
		appendTupleToRelation(relation, mem, 0, t);
	}
	catch (std::string s) {
		fwriteExecute << s <<"\n\n";
		return false;
	}
	return true;
}

bool dropTable(string table_name, ofstream &file_output, SchemaManager &schema_manager) {
	bool result = false;
	try {
		Relation * table_relation = schema_manager.getRelation(table_name);
		if (!table_relation)
		{
			throw table_name + " doesn't exist";
		}
		int num_of_blocks = table_relation->getNumOfBlocks();
		if (num_of_blocks != 0) {
			result = table_relation->deleteBlocks(0);
			if (!result) {
				throw "Deleting from the disk for table " + table_name + " has failed";
			}
		}
		result = schema_manager.deleteRelation(table_name);
		if (!result) {
			throw "Deleting table relation " + table_name + " has failed";
		}
	}
	catch (std::string s) {
		file_output << s <<"\n\n";
		return false;
	}
	return true;
}

void verifySchema(Schema schema, vector<vector<JoinCondition*>> &listOflistOfJoinConditions, string table_name) {
	if (listOflistOfJoinConditions.empty()) {
		return;
	}
	vector<vector<JoinCondition*>>::iterator itrListOfList;
	vector<JoinCondition*>::iterator itrList;
	for (itrListOfList = listOflistOfJoinConditions.begin(); itrListOfList != listOflistOfJoinConditions.end();
		itrListOfList++) {
		for (itrList = (*itrListOfList).begin(); itrList != (*itrListOfList).end();
			itrList++) {
			vector<OperandOperator*> operandVector = (*itrList)->getOperand1();
			vector<OperandOperator*>::iterator operandIterator;
			for (operandIterator = operandVector.begin(); operandIterator != operandVector.end(); operandIterator++) {
				if ((*operandIterator)->getType() == VARIABLE) {
					if (!schema.fieldNameExists((*operandIterator)->getName())) {
						throw "Attribute Name " + (*operandIterator)->getName() + " Does not exist in Table " + (*operandIterator)->getTableName();
					}
				}
			}
		}
	}



}


int getValueFromConversionOfPrefixToInfix(vector<OperandOperator *> vectorOfOperands, Tuple& tuple) {
	stack<int> stackOfIntegers;
	vector<OperandOperator *>::reverse_iterator operandIterator;
	int value = 0, loop = 0;
	for (operandIterator = vectorOfOperands.rbegin(); operandIterator != vectorOfOperands.rend(); operandIterator++) {
		if ((*operandIterator)->getType() == PATTERN) {
			stackOfIntegers.push(stoi((*operandIterator)->getName()));
		}
		else if ((*operandIterator)->getType() == VARIABLE) {
			stackOfIntegers.push(tuple.getField((*operandIterator)->getName()).integer);
		}
		else if ((*operandIterator)->getType() == OPERATOR) {
			int operator1 = stackOfIntegers.top();
			stackOfIntegers.pop();
			int operator2 = stackOfIntegers.top();
			stackOfIntegers.pop();
			if (strcmp((*operandIterator)->getName().c_str(), "+") == 0) {
				stackOfIntegers.push(operator1 + operator2);
			}
			else if (strcmp((*operandIterator)->getName().c_str(), "-") == 0) {
				stackOfIntegers.push(operator1 - operator2);
			}
			else if (strcmp((*operandIterator)->getName().c_str(), "*") == 0) {
				stackOfIntegers.push(operator1 * operator2);
			}

		}
	}
	return stackOfIntegers.top();
}



bool checkIfTupleSatisfiesConditions(Tuple& tuple, Schema& schema, vector<vector<JoinCondition*>> &listOflistOfJoinConditions) {
	if (listOflistOfJoinConditions.empty()) {
		return true;
	}
	vector<vector<JoinCondition*>>::iterator itrListOfList;
	vector<JoinCondition*>::iterator itrList;
	for (itrListOfList = listOflistOfJoinConditions.begin(); itrListOfList != listOflistOfJoinConditions.end(); itrListOfList++) {
		int flag = 1;
		for (itrList = (*itrListOfList).begin(); itrList != (*itrListOfList).end(); itrList++) {

			int size = (*itrList)->getOperand1().size();
			if (size == 1 && (*itrList)->getOperand1().front()->getType() == VARIABLE
				&& schema.getFieldType((*itrList)->getOperand1().front()->getName()) == STR20) {

				string operand1 = *tuple.getField((*itrList)->getOperand1().front()->getName()).str;
				string operand2 = (*itrList)->getOperand2().front()->getType() == VARIABLE ? *tuple.getField((*itrList)->getOperand2().front()->getName()).str :
					(*itrList)->getOperand2().front()->getName();
				if (strcmp((*itrList)->getOperatorOfOperation().c_str(), "<") == 0 && strcmp(operand1.c_str(), operand2.c_str()) >= 0)
				{
					flag = 0;
				}
				else if (strcmp((*itrList)->getOperatorOfOperation().c_str(), "=") == 0 && strcmp(operand1.c_str(), operand2.c_str()) != 0)
				{
					flag = 0;
				}
				else if (strcmp((*itrList)->getOperatorOfOperation().c_str(), ">") == 0 && strcmp(operand1.c_str(), operand2.c_str()) <= 0)
				{
					flag = 0;
				}
			}
			else {
				int valueOfOperand1 = getValueFromConversionOfPrefixToInfix((*itrList)->getOperand1(), tuple);
				int valueOfOperand2 = getValueFromConversionOfPrefixToInfix((*itrList)->getOperand2(), tuple);
				if (strcmp((*itrList)->getOperatorOfOperation().c_str(), "<") == 0 && valueOfOperand1 >= valueOfOperand2)
				{
					flag = 0;
				}
				else if (strcmp((*itrList)->getOperatorOfOperation().c_str(), "=") == 0 && valueOfOperand1 != valueOfOperand2)
				{
					flag = 0;
				}
				else if (strcmp((*itrList)->getOperatorOfOperation().c_str(), ">") == 0 && valueOfOperand1 <= valueOfOperand2)
				{
					flag = 0;
				}
			}

		}
		if (flag == 1) {
			return true;
		}
	}
	return false;
}


Relation * getIntermediateTable(SchemaManager &schema_manager, Schema &schema, vector<OperandOperator*> &projectionList,string table_name,bool renameSchema, std::ofstream& fWriteExecute) {
	vector<OperandOperator*>::iterator itr;
	vector<string> field_names;
	vector<enum FIELD_TYPE> field_types;
	for (itr = projectionList.begin(); itr != projectionList.end(); itr++) {
		if (!renameSchema) {
			field_names.push_back((*itr)->getName());
		}
		else {
			field_names.push_back(table_name + "." + (*itr)->getName());
		}
		field_types.push_back(schema.getFieldType((*itr)->getName()));
	}
	return createTable(schema_manager, getIntermediateTableName(), field_names, field_types, fWriteExecute);
}

void insertIntoIntermediateTable(string table_name, string originalTableName, SchemaManager& schema_manager, Tuple& tuple, MainMemory& mem, vector<OperandOperator*> &projectionList, bool renameSchema, std::ofstream& fWriteExecute){
	vector<OperandOperator*>::iterator itr;
	map<string, string> fieldsToBePassed;
	Schema schemaOfOrginalRelation = tuple.getSchema();
	for (itr = projectionList.begin(); itr != projectionList.end(); itr++) {
		string fieldName = "";
		if (!renameSchema) {
			fieldName = (*itr)->getName();
		}
		else {
			fieldName = originalTableName + "." + (*itr)->getName();
		}
		if (schemaOfOrginalRelation.getFieldType((*itr)->getName()) == STR20) {
			string fieldValue = *(tuple.getField((*itr)->getName()).str);
			fieldsToBePassed.insert(make_pair(fieldName, fieldValue));
		}
		else {
			fieldsToBePassed.insert(make_pair(fieldName, to_string(tuple.getField((*itr)->getName()).integer)));
		}
	}
	insertTable(table_name, schema_manager, fieldsToBePassed, mem, fWriteExecute);
}

bool compareTuple(Tuple& a, Tuple&b, Schema &schema, vector<string>fieldName) {
	vector<string>::iterator itr;
	for (itr = fieldName.begin(); itr != fieldName.end(); itr++) {
		if (schema.getFieldType(*itr) == STR20) {
			int value = strcmp((*a.getField(*itr).str).c_str(), (*b.getField(*itr).str).c_str());
			if (value > 0) {
				return true;
			}
			else if (value<0) {
				return false;
			}

		}
		else if (schema.getFieldType(*itr) == INT) {
			if (a.getField(*itr).integer > b.getField(*itr).integer) {
				return true;
			}
			else if (a.getField(*itr).integer < b.getField(*itr).integer) {
				return false;
			}
		}
	}
	return false;
}


void mergeTuples(vector<Tuple>& list1, vector<Tuple>& list2, vector<Tuple>& mergeList, vector<OperandOperator*>& attributesList, Schema& schema)
{
	int i = 0, j = 0;
	mergeList.clear();
	while (i < list1.size() && j < list2.size())
	{
		Tuple t1 = list1[i];
		Tuple t2 = list2[j];
		for (vector<OperandOperator*>::iterator operIter = attributesList.begin(); operIter != attributesList.end(); operIter++)
		{
			if (schema.getFieldType((*operIter)->getName()) == STR20)
			{
				string val1 = *(t1.getField((*operIter)->getName()).str);
				string val2 = *(t2.getField((*operIter)->getName()).str);
				int compareVal = strcmp(val1.c_str(), val2.c_str());
				if (compareVal < 0 || ((operIter+1)==attributesList.end() && compareVal<=0))
				{
					mergeList.push_back(t1);
					i++;
					break;
				}
				if (compareVal > 0)
				{
					mergeList.push_back(t2);
					j++;
					break;
				}
			}
			else
			{
				int val1 = t1.getField((*operIter)->getName()).integer;
				int val2 = t2.getField((*operIter)->getName()).integer;
				if (val1 < val2 || ((operIter + 1) == attributesList.end() && val1 <= val2))
				{
					mergeList.push_back(t1);
					i++;
					break;
				}
				if (val1 > val2)
				{
					mergeList.push_back(t2);
					j++;
					break;
				}
			}
		}
	}
	while (i < list1.size())
		mergeList.push_back(list1[i++]);
	while (j < list2.size())
		mergeList.push_back(list2[j++]);
}

void mergeSortTuples(vector<Tuple>& listOfTuples, vector<OperandOperator*>& attributesList, Schema& schema)
{
	vector<Tuple> list1, list2;
	int i;
	for (i = 0; i < ((listOfTuples.size()) / 2); i++)
		list1.push_back(listOfTuples[i]);
	for (i = ((listOfTuples.size()) / 2); i < listOfTuples.size(); i++)
		list2.push_back(listOfTuples[i]);
	if (list1.size() > 0 && list2.size() > 0)
	{
		mergeSortTuples(list1, attributesList, schema);
		mergeSortTuples(list2, attributesList, schema);
		mergeTuples(list1, list2, listOfTuples, attributesList, schema);
	}
}
bool tupleLeftToProcess(MainMemory& mem, int* count, int sizeOfSubList)
{
	for (int i = 0; i < sizeOfSubList; i++)
	{
		if (count[i] < mem.getBlock(i)->getNumTuples())
			return true;
	}
	return false;
}

int getLeastBlock(vector<Tuple>& tuples, vector<int>& blockNum, vector<OperandOperator*>& attributesList, Schema& schema)
{
	Tuple minTuple = tuples[0];
	int minTupleCount = 0;
	vector<string> fieldNames;
	for (vector<OperandOperator*>::iterator iter = attributesList.begin(); iter != attributesList.end(); iter++)
		fieldNames.push_back((*iter)->getName());
	for (int tupleCount = 1; tupleCount < tuples.size(); tupleCount++)
	{
		if (compareTuple(minTuple, tuples[tupleCount], schema, fieldNames))
		{
			minTupleCount = tupleCount;
			minTuple = tuples[tupleCount];
		}
	}
	return blockNum[minTupleCount];
}

Relation* mergeSubList(Relation* relationPtr, SchemaManager &schema_manager, int sizeOfSubList, vector<OperandOperator*>& attributesList, MainMemory& mem, std::ofstream& fWriteExecute)
{
	Schema schema = relationPtr->getSchema();
	int count[NUM_OF_BLOCKS_IN_MEMORY];
	for (int i = 0; i < sizeOfSubList; i++)
		count[i] = 0;
	Block* outputBlock = mem.getBlock(NUM_OF_BLOCKS_IN_MEMORY - 1);
	outputBlock->clear();
	Relation* outputRelation = createTable(schema_manager, getIntermediateTableName(), schema.getFieldNames(), schema.getFieldTypes(), fWriteExecute);
	int blockCount = 0;
	while (tupleLeftToProcess(mem, count, sizeOfSubList))
	{
		vector<Tuple> tuples;
		vector<int> blockNumbers;
		for (int memBlockNumber = 0; memBlockNumber < sizeOfSubList; memBlockNumber++)
		{
			Block* blockPtr = mem.getBlock(memBlockNumber);
			if (count[memBlockNumber] < blockPtr->getNumTuples())
			{
				tuples.push_back(blockPtr->getTuple(count[memBlockNumber]));
				blockNumbers.push_back(memBlockNumber);
			}
		}
		int minBlockNumber = getLeastBlock(tuples, blockNumbers, attributesList, schema);
		Block* minBlock = mem.getBlock(minBlockNumber);
		if (outputBlock->isFull())
		{
			outputRelation->setBlock(blockCount++, NUM_OF_BLOCKS_IN_MEMORY - 1);
			outputBlock->clear();
		}
		outputBlock->appendTuple(minBlock->getTuple(count[minBlockNumber]));
		count[minBlockNumber]++;
	}
	outputRelation->setBlock(blockCount, NUM_OF_BLOCKS_IN_MEMORY - 1);
	outputBlock->clear();
	return outputRelation;
}

vector<Relation*> sortSubList(Relation* relationPtr, SchemaManager &schema_manager, int sizeOfSubList, vector<OperandOperator*>& attributesList, MainMemory& mem, bool& success, std::ofstream& fWriteExecute)
{
	Schema schema = relationPtr->getSchema();
	vector<Relation*> subLists;
	for (vector<OperandOperator*>::iterator iter = attributesList.begin(); iter != attributesList.end(); iter++)
	{
		if (!schema.fieldNameExists((*iter)->getName()))
		{
			success = false;
			return subLists;
		}
	}
	int blocksCount = relationPtr->getNumOfBlocks();
	for (int i = 0; i < blocksCount / sizeOfSubList; i++)
	{
		for (int j = 0; j < sizeOfSubList; j++)
		{
			Block *blockPtr = mem.getBlock(j);
			blockPtr->clear();

			relationPtr->getBlock(i*sizeOfSubList + j, j);
			vector<Tuple>listOfTuples = blockPtr->getTuples();
			mergeSortTuples(listOfTuples, attributesList, schema);
			blockPtr->setTuples(listOfTuples);
		}
		subLists.push_back(mergeSubList(relationPtr, schema_manager, sizeOfSubList, attributesList, mem, fWriteExecute));
	}
	if ((blocksCount%sizeOfSubList) != 0)
	{
		for (int j = 0; j < blocksCount%sizeOfSubList; j++)
		{
			Block *blockPtr = mem.getBlock(j);
			blockPtr->clear();
			int blockIndex = (blocksCount / sizeOfSubList)*sizeOfSubList + j;
			relationPtr->getBlock(blockIndex, j);
			vector<Tuple>listOfTuples = blockPtr->getTuples();
			mergeSortTuples(listOfTuples, attributesList, schema);
			blockPtr->setTuples(listOfTuples);
		}
		subLists.push_back(mergeSubList(relationPtr, schema_manager, blocksCount%sizeOfSubList, attributesList, mem, fWriteExecute));
	}
	success = true;
	return subLists;
}

Relation* selectTable(string table_name, SchemaManager &schema_manager, vector<vector<JoinCondition*>> &listOfJoinConditions, MainMemory& mem, vector<OperandOperator*> &projectionList, bool renameSchema, std::ofstream& fwriteExecute) {
	Relation * intermediate_table = NULL;
	try {
		Relation *table_relation = schema_manager.getRelation(table_name);
		if (table_relation == NULL) {
			throw "Given relation with " + table_name + " Does not exist";
		}
		Schema schema = table_relation->getSchema();
			if (projectionList.empty()) {
				vector<string> fieldNames = schema.getFieldNames();
				for (vector<string>::iterator iter = fieldNames.begin(); iter != fieldNames.end(); iter++)
				{
					OperandOperator* obj = new OperandOperator((*iter), kOperandOperatorType::VARIABLE, table_name);
					projectionList.push_back(obj);
				}
			}
		intermediate_table = getIntermediateTable(schema_manager, schema, projectionList,table_name,renameSchema, fwriteExecute);
		verifySchema(schema, listOfJoinConditions, table_name);
		Block *block_pointer = mem.getBlock(0);
		int numOfBlocks = table_relation->getNumOfBlocks();
		for (int i = 0; i < numOfBlocks; i++) {
			block_pointer->clear();
			table_relation->getBlock(i, 0);
			vector<Tuple> listOfTuples = block_pointer->getTuples();
			vector<Tuple>::iterator itr;
			for (itr = listOfTuples.begin(); itr != listOfTuples.end(); itr++) {
				Tuple tuple = *itr;
				bool resultOfCheckingOnConditions = checkIfTupleSatisfiesConditions(tuple, schema, listOfJoinConditions);
				if (resultOfCheckingOnConditions) {
					insertIntoIntermediateTable(intermediate_table->getRelationName(), table_name, schema_manager, tuple, mem, projectionList,renameSchema, fwriteExecute);
				}
			}

		}

	}
	catch (std::string s) {
		fwriteExecute << s<<"\n\n";
		return NULL;
	}
	return intermediate_table;
}



Relation * sortOperation(vector<Relation*>  vectorOfSubLists, SchemaManager& schema_manager, MainMemory& mem, vector<string> fieldName) {
	vector<Relation *>::iterator itr;
	Relation *sorted_table = NULL;
	int relation_block_index = 0;
	int counter = 0;
	map<Relation*, int> mapOfRelationNamesWithBlocks;
	map<int, Relation*> memoryBlockIndex;
	map<int, int> memoryTupleIndex;
	Schema schema;
	for (itr = vectorOfSubLists.begin(); itr != vectorOfSubLists.end(); itr++) {
		if (counter == 0) {
			Schema schemaOfSortedRelation = (*itr)->getSchema();
			sorted_table = schema_manager.createRelation(getIntermediateTableName(), schemaOfSortedRelation);
			schema = sorted_table->getSchema();
		}
		string relName = (*itr)->getRelationName();
		mapOfRelationNamesWithBlocks.insert(make_pair(*itr, 0));
		memoryBlockIndex.insert(make_pair(counter, *itr));
		memoryTupleIndex.insert(make_pair(counter, 0));
		(*itr)->getBlock(0, counter);
		++counter;
	}
	Block* resultant_block_pointer = mem.getBlock(mem.getMemorySize() - 1);
	resultant_block_pointer->clear();
	int totalNoofTables = mapOfRelationNamesWithBlocks.size();
	int noOfTablesComplete = 0;
	Tuple minimumTuple = mem.getBlock(0)->getTuple(0);
	int minimumBlockIndex = 0;
	int minimumTupleIndex = 0;
	while (noOfTablesComplete < totalNoofTables) {
		for (int i = 0; i <totalNoofTables; i++) {
			if (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
				Block* block_pointer = mem.getBlock(i);
				int numberOfTuples = block_pointer->getNumTuples();
				if (memoryTupleIndex[i] >= numberOfTuples) {
					Relation* relationOfBlock = memoryBlockIndex[i];
					int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
					if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
						++noOfTablesComplete;
						continue;
					}
					else {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
						relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
						memoryTupleIndex.at(i) = 0;
					}
				}
				if (compareTuple(minimumTuple, block_pointer->getTuple(memoryTupleIndex[i]), schema, fieldName) == true) {
					minimumBlockIndex = i;
					minimumTupleIndex = memoryTupleIndex[i];
					minimumTuple = block_pointer->getTuple(memoryTupleIndex[i]);
				}
			}
		}

		if (resultant_block_pointer->isFull()) {
			sorted_table->setBlock(relation_block_index++, mem.getMemorySize() - 1);
			resultant_block_pointer->clear();
		}
		resultant_block_pointer->appendTuple(minimumTuple);
		memoryTupleIndex.at(minimumBlockIndex) = minimumTupleIndex + 1;

		//Next first tuple
		for (int i = 0; i < totalNoofTables; i++) {
			if (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
				Relation * relation_pointer = memoryBlockIndex[i];
				if (memoryTupleIndex[i] < mem.getBlock(i)->getNumTuples()) {
					minimumTuple = mem.getBlock(i)->getTuple(memoryTupleIndex[i]);
					minimumBlockIndex = i;
					minimumTupleIndex = memoryTupleIndex[i];
					break;
				}
				else if (memoryTupleIndex[i] >= mem.getBlock(i)->getNumTuples()) {
					Relation* relationOfBlock = memoryBlockIndex[i];
					int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
					if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
						++noOfTablesComplete;
						continue;
					}
					else {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
						relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
						memoryTupleIndex.at(i) = 0;
						minimumTuple = mem.getBlock(i)->getTuple(memoryTupleIndex[i]);
						minimumBlockIndex = i;
						minimumTupleIndex = memoryTupleIndex[i];
						break;
					}
				}
			}

		}
	}
	if (!resultant_block_pointer->isEmpty()) {
		sorted_table->setBlock(relation_block_index++, mem.getMemorySize() - 1);
		resultant_block_pointer->clear();
	}
	return sorted_table;
}

bool compareEquality(Tuple& a, Tuple&b, Schema &schema, vector<string>fieldName) {
	vector<string>::iterator itr;
	for (itr = fieldName.begin(); itr != fieldName.end(); itr++) {
		if (schema.getFieldType(*itr) == STR20) {
			int value = strcmp((*a.getField(*itr).str).c_str(), (*b.getField(*itr).str).c_str());
			if (value != 0) {
				return false;
			}
		}
		else if (schema.getFieldType(*itr) == INT) {
			if (a.getField(*itr).integer != b.getField(*itr).integer) {
				return false;
			}
		}
	}
	return true;
}




Relation * removeDuplicatesOperation(vector<Relation*>  vectorOfSubLists, SchemaManager& schema_manager, MainMemory& mem, vector<string> fieldName) {
	vector<Relation *>::iterator itr;
	Relation *duplicate_removal = NULL;
	int relation_block_index = 0;
	int counter = 0;
	map<Relation*, int> mapOfRelationNamesWithBlocks;
	map<int, Relation*> memoryBlockIndex;
	map<int, int> memoryTupleIndex;
	Schema schema;
	for (itr = vectorOfSubLists.begin(); itr != vectorOfSubLists.end(); itr++) {
		if (counter == 0) {
			Schema schemaOfSortedRelation = (*itr)->getSchema();
			duplicate_removal = schema_manager.createRelation(getIntermediateTableName(), schemaOfSortedRelation);
			schema = duplicate_removal->getSchema();
		}
		string relName = (*itr)->getRelationName();
		mapOfRelationNamesWithBlocks.insert(make_pair(*itr, 0));
		memoryBlockIndex.insert(make_pair(counter, *itr));
		memoryTupleIndex.insert(make_pair(counter, 0));
		(*itr)->getBlock(0, counter);
		++counter;
	}
	Block* resultant_block_pointer = mem.getBlock(mem.getMemorySize() - 1);
	resultant_block_pointer->clear();
	int totalNoofTables = mapOfRelationNamesWithBlocks.size();
	int noOfTablesComplete = 0;
	Tuple minimumTuple = mem.getBlock(0)->getTuple(0);
	int minimumBlockIndex = 0;
	int minimumTupleIndex = 0;
	while (noOfTablesComplete < totalNoofTables) {

		for (int i = 0; i < totalNoofTables; i++) {
			if (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
				Block* block_pointer = mem.getBlock(i);
				int numberOfTuples = block_pointer->getNumTuples();
				if (memoryTupleIndex[i] >= numberOfTuples) {
					Relation* relationOfBlock = memoryBlockIndex[i];
					int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
					if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
						++noOfTablesComplete;
						continue;
					}
					else {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
						relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
						memoryTupleIndex.at(i) = 0;
					}
				}
				if (compareTuple(minimumTuple, block_pointer->getTuple(memoryTupleIndex[i]), schema, fieldName) == true) {
					minimumBlockIndex = i;
					minimumTupleIndex = memoryTupleIndex[i];
					minimumTuple = block_pointer->getTuple(memoryTupleIndex[i]);
				}
			}
		}
		for (int i = 0; i <totalNoofTables; i++) {
			if (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
				Block* block_pointer = mem.getBlock(i);
				int numberOfTuples = block_pointer->getNumTuples();
				if (memoryTupleIndex[i] >= numberOfTuples) {
					Relation* relationOfBlock = memoryBlockIndex[i];
					int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
					if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
						++noOfTablesComplete;
						continue;
					}
					else {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
						relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
						memoryTupleIndex.at(i) = 0;
					}
				}
				while (compareEquality(minimumTuple, block_pointer->getTuple(memoryTupleIndex[i]), schema, fieldName) == true) {
					int tupleIndex = memoryTupleIndex[i];
					memoryTupleIndex.at(i) = tupleIndex + 1;
					if (memoryTupleIndex[i] >= numberOfTuples) {
						Relation* relationOfBlock = memoryBlockIndex[i];
						int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
						if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
							mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
							++noOfTablesComplete;
							break;
						}
						else {
							mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
							relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
							numberOfTuples = block_pointer->getNumTuples();
							memoryTupleIndex.at(i) = 0;
						}
					}
				}
			}
		}

		if (resultant_block_pointer->isFull()) {
			duplicate_removal->setBlock(relation_block_index++, mem.getMemorySize() - 1);
			resultant_block_pointer->clear();
		}
		resultant_block_pointer->appendTuple(minimumTuple);
		//Next first tuple
		for (int i = 0; i < totalNoofTables; i++) {
			if (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
				if (memoryTupleIndex[i] < mem.getBlock(i)->getNumTuples()) {
					minimumTuple = mem.getBlock(i)->getTuple(memoryTupleIndex[i]);
					minimumBlockIndex = i;
					minimumTupleIndex = memoryTupleIndex[i];
					break;
				}
				else if (memoryTupleIndex[i] >= mem.getBlock(i)->getNumTuples()) {
					Relation* relationOfBlock = memoryBlockIndex[i];
					int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
					if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
						++noOfTablesComplete;
						continue;
					}
					else {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
						relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
						memoryTupleIndex.at(i) = 0;
						minimumTuple = mem.getBlock(i)->getTuple(memoryTupleIndex[i]);
						minimumBlockIndex = i;
						minimumTupleIndex = memoryTupleIndex[i];
						break;
					}
				}
			}
		}
	}
	if (!resultant_block_pointer->isEmpty()) {
		duplicate_removal->setBlock(relation_block_index++, mem.getMemorySize() - 1);
		resultant_block_pointer->clear();
	}
	return duplicate_removal;
}

bool checkIfTupleSatisfiesJoinConditions(Tuple& tupleForTable1,Tuple& tupleForTable2, Schema& schemaOfTable1,vector<JoinCondition*> &listOfJoinConditions) {
	if (listOfJoinConditions.empty()) {
		return true;
	}
	vector<JoinCondition*>::iterator itrList;
		for (itrList = listOfJoinConditions.begin(); itrList != listOfJoinConditions.end(); itrList++) {
			int size = (*itrList)->getOperand1().size();
			if (size == 1 && (*itrList)->getOperand1().front()->getType() == VARIABLE
				&& schemaOfTable1.getFieldType((*itrList)->getOperand1().front()->getName()) == STR20) {

				string operand1 = *tupleForTable1.getField((*itrList)->getOperand1().front()->getName()).str;
				string operand2 = (*itrList)->getOperand2().front()->getType() == VARIABLE ? *tupleForTable2.getField((*itrList)->getOperand2().front()->getName()).str :
					(*itrList)->getOperand2().front()->getName();
				if (strcmp((*itrList)->getOperatorOfOperation().c_str(), "<") == 0 && strcmp(operand1.c_str(), operand2.c_str()) >= 0)
				{
					return false;
				}
				else if (strcmp((*itrList)->getOperatorOfOperation().c_str(), "=") == 0 && strcmp(operand1.c_str(), operand2.c_str()) != 0)
				{
					return false;
				}
				else if (strcmp((*itrList)->getOperatorOfOperation().c_str(), ">") == 0 && strcmp(operand1.c_str(), operand2.c_str()) <= 0)
				{
					return false;
				}
			}
			else {
				int valueOfOperand1 = getValueFromConversionOfPrefixToInfix((*itrList)->getOperand1(), tupleForTable1);
				int valueOfOperand2 = getValueFromConversionOfPrefixToInfix((*itrList)->getOperand2(), tupleForTable2);
				if (strcmp((*itrList)->getOperatorOfOperation().c_str(), "<") == 0 && valueOfOperand1 >= valueOfOperand2)
				{
					return false;
				}
				else if (strcmp((*itrList)->getOperatorOfOperation().c_str(), "=") == 0 && valueOfOperand1 != valueOfOperand2)
				{
					return false;
				}
				else if (strcmp((*itrList)->getOperatorOfOperation().c_str(), ">") == 0 && valueOfOperand1 <= valueOfOperand2)
				{
					return false;
				}
			}
	}
		return true;
}


bool checkMinimumTupleAcrossSchemas(Tuple& tupleForTable1, Tuple& tupleForTable2, 
	Schema& schemaOfTable1, vector<JoinCondition*> &listOfJoinConditions) {
	if (listOfJoinConditions.empty()) {
		return true;
	}
	vector<JoinCondition*>::iterator itrList;
	for (itrList = listOfJoinConditions.begin(); itrList != listOfJoinConditions.end(); itrList++) {
		int size = (*itrList)->getOperand1().size();
		if (size == 1 && (*itrList)->getOperand1().front()->getType() == VARIABLE
			&& schemaOfTable1.getFieldType((*itrList)->getOperand1().front()->getName()) == STR20) {

			string operand1 = *tupleForTable1.getField((*itrList)->getOperand1().front()->getName()).str;
			string operand2 = (*itrList)->getOperand2().front()->getType() == VARIABLE ? *tupleForTable2.getField((*itrList)->getOperand2().front()->getName()).str :
				(*itrList)->getOperand2().front()->getName();
			if (strcmp(operand1.c_str(), operand2.c_str()) > 0)
			{
				return true;
			}
			else if (strcmp(operand1.c_str(), operand2.c_str()) < 0)
			{
				return false;
			}
		}
		else {
			int valueOfOperand1 = getValueFromConversionOfPrefixToInfix((*itrList)->getOperand1(), tupleForTable1);
			int valueOfOperand2 = getValueFromConversionOfPrefixToInfix((*itrList)->getOperand2(), tupleForTable2);
			if (valueOfOperand1 >valueOfOperand2)
			{
				return true;
			}
			else if (valueOfOperand1 <valueOfOperand2)
			{
				return false;
			}
		}
	}
	return false;
}

vector<string> getStringOfVariablesFromConversionOfPrefixToInfix(vector<OperandOperator *> vectorOfOperands) {
	vector<string> fieldNames;
	vector<OperandOperator *>::reverse_iterator operandIterator;
	for (operandIterator = vectorOfOperands.rbegin(); operandIterator != vectorOfOperands.rend(); operandIterator++) {
		if ((*operandIterator)->getType() == VARIABLE) {
			fieldNames.push_back((*operandIterator)->getName());
		}
	}
	return fieldNames;
}

vector<string> getListOfVariablesToCompareForMinimumTupleFromTable(Schema & schema,vector<JoinCondition*> &listOfJoinConditions, int index) {
	vector<string> vectorOfAttributesOfTable;
	vector<JoinCondition*>::iterator itrList;
	if (index == 1) {
		for (itrList = listOfJoinConditions.begin(); itrList != listOfJoinConditions.end(); itrList++) {
			int size = (*itrList)->getOperand1().size();
			if (size == 1 && (*itrList)->getOperand1().front()->getType() == VARIABLE
				&& schema.getFieldType((*itrList)->getOperand1().front()->getName()) == STR20) {
				vectorOfAttributesOfTable.push_back((*itrList)->getOperand1().front()->getName());
			}
			else {
				vector<string>attributeList = getStringOfVariablesFromConversionOfPrefixToInfix((*itrList)->getOperand1());
				vectorOfAttributesOfTable.insert(vectorOfAttributesOfTable.end(), attributeList.begin(), attributeList.end());
			}
		}
	}
	else {
		for (itrList = listOfJoinConditions.begin(); itrList != listOfJoinConditions.end(); itrList++) {
			int size = (*itrList)->getOperand2().size();
			if (size == 1 && (*itrList)->getOperand2().front()->getType() == VARIABLE
				&& schema.getFieldType((*itrList)->getOperand2().front()->getName()) == STR20) {
				vectorOfAttributesOfTable.push_back((*itrList)->getOperand2().front()->getName());
			}
			else {
				vector<string>attributeList = getStringOfVariablesFromConversionOfPrefixToInfix((*itrList)->getOperand2());
				vectorOfAttributesOfTable.insert(vectorOfAttributesOfTable.end(), attributeList.begin(), attributeList.end());
			}
		}
	}
	return vectorOfAttributesOfTable;
}

Tuple joinTuples(Relation *joinResultTuple, Tuple& tupleOfTable1, Tuple& tupleOfTable2) {
	Tuple& resultantTuple=joinResultTuple->createTuple();
	vector<string>::iterator itr;
	Schema schemaOfTable1 = tupleOfTable1.getSchema();
	Schema schemaOfTable2 = tupleOfTable2.getSchema();
	vector<string> fieldNamesOfTable1 = schemaOfTable1.getFieldNames();
	vector<string> fieldNamesOfTable2 = schemaOfTable2.getFieldNames();
	for (itr = fieldNamesOfTable1.begin(); itr != fieldNamesOfTable1.end(); itr++) {
		if (schemaOfTable1.getFieldType(*itr) == STR20) {
			resultantTuple.setField(*itr, (*tupleOfTable1.getField(*itr).str));
		}
		else {
			resultantTuple.setField(*itr,tupleOfTable1.getField(*itr).integer);
		}
	}
	for (itr = fieldNamesOfTable2.begin(); itr != fieldNamesOfTable2.end(); itr++) {
		if (schemaOfTable2.getFieldType(*itr) == STR20) {
			resultantTuple.setField(*itr, (*tupleOfTable2.getField(*itr).str));
		}
		else {
			resultantTuple.setField(*itr,tupleOfTable2.getField(*itr).integer);
		}
	}
	return resultantTuple;
}



Relation * createSchemaForJoinTable(vector<Relation*> subListsOfTable1, vector<Relation*> subListsOfTable2, SchemaManager& schemaManager) {
	Relation * firstTupleOfSubListTable1 = subListsOfTable1.front();
	Schema schemaOfTable1 = firstTupleOfSubListTable1->getSchema();
	Relation * firstTupleOfSubListTable2 = subListsOfTable2.front();
	Schema schemaOfTable2 = firstTupleOfSubListTable2->getSchema();
	vector<string> vectorOfFieldNamesTable1 = schemaOfTable1.getFieldNames();
	vector<string> vectorOfFieldNamesTable2 = schemaOfTable2.getFieldNames();
	vector<FIELD_TYPE> vectorOfFieldTypesTable1 = schemaOfTable1.getFieldTypes();
	vector<FIELD_TYPE> vectorOfFieldTypesTable2 = schemaOfTable2.getFieldTypes();
	vectorOfFieldNamesTable1.insert(vectorOfFieldNamesTable1.end(), vectorOfFieldNamesTable2.begin(), vectorOfFieldNamesTable2.end());
    vectorOfFieldTypesTable1.insert(vectorOfFieldTypesTable1.end(), vectorOfFieldTypesTable2.begin(), vectorOfFieldTypesTable2.end());
	Schema joinSchema(vectorOfFieldNamesTable1, vectorOfFieldTypesTable1);
	Relation * joinRelation = schemaManager.createRelation(getIntermediateTableName(), joinSchema);
	return joinRelation;
}


Relation * joinTables(vector<Relation*>& subListsOfTable1, vector<Relation*>& subListsOfTable2, 
	SchemaManager &schema_manager,MainMemory &mem,vector<JoinCondition*>& joinConditions,Relation *joinedTable) {
	
	Schema schemaOfTable1 = subListsOfTable1.front()->getSchema();
	Schema schemaOfTable2 = subListsOfTable2.front()->getSchema();
	vector<Relation *>::iterator itr;
	int relation_block_index = joinedTable->getNumOfBlocks();
	int counter = 0;// No of 
	map<Relation*, int> mapOfRelationNamesWithBlocks;
	map<int, Relation*> memoryBlockIndex;
	map<int, int> memoryTupleIndex;
	for (itr = subListsOfTable1.begin(); itr != subListsOfTable1.end(); itr++) {
		mapOfRelationNamesWithBlocks.insert(make_pair(*itr, 0));
		memoryBlockIndex.insert(make_pair(counter, *itr));
		memoryTupleIndex.insert(make_pair(counter, 0));
		(*itr)->getBlock(0, counter);
		++counter;
	}
	for (itr = subListsOfTable2.begin(); itr != subListsOfTable2.end(); itr++) {
		mapOfRelationNamesWithBlocks.insert(make_pair(*itr, 0));
		memoryBlockIndex.insert(make_pair(counter, *itr));
		memoryTupleIndex.insert(make_pair(counter, 0));
		(*itr)->getBlock(0, counter);
		++counter;
	}
	vector<string> comparingJoinOperandsOfTable1 = getListOfVariablesToCompareForMinimumTupleFromTable(subListsOfTable1.front()->getSchema(), joinConditions,1);
	vector<string> comparingJoinOperandsOfTable2 = getListOfVariablesToCompareForMinimumTupleFromTable(subListsOfTable2.front()->getSchema(), joinConditions, 2);
	

	
	Block* resultant_block_pointer = mem.getBlock(mem.getMemorySize() - 1);
	resultant_block_pointer->clear();
	int totalNoofTables1 = subListsOfTable1.size();
	int totalNoofTables2 = subListsOfTable2.size();
	int noOfTables1Complete = 0;
	int noOfTables2Complete = 0;
	Tuple minimumTuple1 = mem.getBlock(0)->getTuple(0);
	int minimumBlockIndex1 = 0;
	int minimumTupleIndex1 = 0;
	bool minimumTupleSelectedFrom1 = false;
	Tuple minimumTuple2 = mem.getBlock(subListsOfTable1.size())->getTuple(0);
	int minimumBlockIndex2 = subListsOfTable1.size();
	int minimumTupleIndex2 = 0;
	while (noOfTables1Complete < totalNoofTables1 && noOfTables2Complete<totalNoofTables2) {


		for (int i = 0; i <totalNoofTables1; i++) {
			if (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
				Block* block_pointer = mem.getBlock(i);
				int numberOfTuples = block_pointer->getNumTuples();
				if (memoryTupleIndex[i] >= numberOfTuples) {
					Relation* relationOfBlock = memoryBlockIndex[i];
					int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
					if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
						++noOfTables1Complete;
						continue;
					}
					else {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
						relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
						memoryTupleIndex.at(i) = 0;
					}
				}
				if (compareTuple(minimumTuple1, block_pointer->getTuple(memoryTupleIndex[i]),subListsOfTable1.front()->getSchema(), comparingJoinOperandsOfTable1) == true) {
					minimumBlockIndex1 = i;
					minimumTupleIndex1 = memoryTupleIndex[i];
					minimumTuple1 = block_pointer->getTuple(memoryTupleIndex[i]);
				}
			}
		}


		for (int i = subListsOfTable1.size(); i < subListsOfTable1.size() + subListsOfTable2.size(); i++) {
			if (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
				Block* block_pointer = mem.getBlock(i);
				int numberOfTuples = block_pointer->getNumTuples();
				if (memoryTupleIndex[i] >= numberOfTuples) {
					Relation* relationOfBlock = memoryBlockIndex[i];
					int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
					if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
						++noOfTables2Complete;
						continue;
					}
					else {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
						relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
						memoryTupleIndex.at(i) = 0;
					}
				}
				if (compareTuple(minimumTuple2, block_pointer->getTuple(memoryTupleIndex[i]), subListsOfTable2.front()->getSchema(), comparingJoinOperandsOfTable2) == true) {
					minimumBlockIndex2 = i;
					minimumTupleIndex2 = memoryTupleIndex[i];
					minimumTuple2 = block_pointer->getTuple(memoryTupleIndex[i]);
				}
			}
		}
		Tuple minimumTuple = minimumTuple1;
		int minimumBlockIndex = minimumBlockIndex1;
		int minimumTupleIndex = minimumTupleIndex1;
		if (checkMinimumTupleAcrossSchemas(minimumTuple1,minimumTuple2,schemaOfTable1,joinConditions)==false){
			minimumTupleSelectedFrom1 = true;
		}
		else {
			minimumTuple = minimumTuple2;
			minimumBlockIndex = minimumBlockIndex2;
			minimumTupleIndex = minimumTupleIndex2;
			minimumTupleSelectedFrom1 = false;
		}
		
		if (minimumTupleSelectedFrom1 == true) {
			vector<Tuple> listOfTuplesOfTable2WhichMatchWithTable1;
			for (int i = subListsOfTable1.size(); i < subListsOfTable1.size() + subListsOfTable2.size(); i++) {
				while (mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
					Block* block_pointer = mem.getBlock(i);
					int numberOfTuples = block_pointer->getNumTuples();
					if (memoryTupleIndex[i] >= numberOfTuples) {
						Relation* relationOfBlock = memoryBlockIndex[i];
						int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
						if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
							mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
							++noOfTables2Complete;
							continue;
						}
						else {
							mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
							relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
							memoryTupleIndex.at(i) = 0;
						}
					}
					if (checkIfTupleSatisfiesJoinConditions(minimumTuple, block_pointer->getTuple(memoryTupleIndex[i]), schemaOfTable1, joinConditions) == true) {
						listOfTuplesOfTable2WhichMatchWithTable1.push_back(block_pointer->getTuple(memoryTupleIndex[i]));
						memoryTupleIndex.at(i) = memoryTupleIndex.at(i) + 1;
					}
					else {
						break;
					}
				}

			}
			if (listOfTuplesOfTable2WhichMatchWithTable1.size() > 0) {
				for (int i = 0; i < totalNoofTables1; i++) {
					while (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
						Block* block_pointer = mem.getBlock(i);
						int numberOfTuples = block_pointer->getNumTuples();
						if (memoryTupleIndex[i] >= numberOfTuples) {
							Relation* relationOfBlock = memoryBlockIndex[i];
							int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
							if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
								mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
								++noOfTables1Complete;
								continue;
							}
							else {
								mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
								relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
								memoryTupleIndex.at(i) = 0;
							}
						}
						if (compareEquality(minimumTuple, block_pointer->getTuple(memoryTupleIndex[i]), subListsOfTable1.front()->getSchema(), comparingJoinOperandsOfTable1) == true) {
							vector<Tuple>::iterator itr;
							for (itr = listOfTuplesOfTable2WhichMatchWithTable1.begin(); itr != listOfTuplesOfTable2WhichMatchWithTable1.end(); itr++) {
								if (resultant_block_pointer->isFull()) {
									joinedTable->setBlock(relation_block_index++, mem.getMemorySize() - 1);
									resultant_block_pointer->clear();
								}
								Tuple newTuple = joinTuples(joinedTable, block_pointer->getTuple(memoryTupleIndex[i]), *itr);
								resultant_block_pointer->appendTuple(newTuple);
							}
							memoryTupleIndex.at(i) = memoryTupleIndex[i] + 1;
						}
						else {
							break;
						}
					}
				}
			}
			else {
				memoryTupleIndex.at(minimumBlockIndex) = minimumTupleIndex + 1;
			}
		}
		else {
			vector<Tuple> listOfTuplesOfTable1WhichMatchWithTable2;
			for (int i =0; i < subListsOfTable1.size(); i++) {
				while (mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
					Block* block_pointer = mem.getBlock(i);
					int numberOfTuples = block_pointer->getNumTuples();
					if (memoryTupleIndex[i] >= numberOfTuples) {
						Relation* relationOfBlock = memoryBlockIndex[i];
						int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
						if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
							mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
							++noOfTables1Complete;
							continue;
						}
						else {
							mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
							relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
							memoryTupleIndex.at(i) = 0;
						}
					}
					if (checkIfTupleSatisfiesJoinConditions(block_pointer->getTuple(memoryTupleIndex[i]),minimumTuple, schemaOfTable1, joinConditions) == true) {
						listOfTuplesOfTable1WhichMatchWithTable2.push_back(block_pointer->getTuple(memoryTupleIndex[i]));
						memoryTupleIndex.at(i) = memoryTupleIndex.at(i) + 1;
					}
					else {
						break;
					}
				}

			}
			if (listOfTuplesOfTable1WhichMatchWithTable2.size() > 0) {
				for (int i = subListsOfTable1.size(); i < subListsOfTable1.size() + subListsOfTable2.size(); i++) {
					while (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
						Block* block_pointer = mem.getBlock(i);
						int numberOfTuples = block_pointer->getNumTuples();
						if (memoryTupleIndex[i] >= numberOfTuples) {
							Relation* relationOfBlock = memoryBlockIndex[i];
							int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
							if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
								mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
								++noOfTables2Complete;
								continue;
							}
							else {
								mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
								relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
								memoryTupleIndex.at(i) = 0;
							}
						}
						if (compareEquality(minimumTuple, block_pointer->getTuple(memoryTupleIndex[i]),schemaOfTable2, comparingJoinOperandsOfTable2) == true) {
							vector<Tuple>::iterator itr;
							for (itr = listOfTuplesOfTable1WhichMatchWithTable2.begin(); itr != listOfTuplesOfTable1WhichMatchWithTable2.end(); itr++) {
								if (resultant_block_pointer->isFull()) {
									joinedTable->setBlock(relation_block_index++, mem.getMemorySize() - 1);
									resultant_block_pointer->clear();
								}
								Tuple newTuple = joinTuples(joinedTable,*itr, block_pointer->getTuple(memoryTupleIndex[i]));
								resultant_block_pointer->appendTuple(newTuple);
							}
							memoryTupleIndex.at(i) = memoryTupleIndex[i] + 1;
						}
						else {
							break;
						}
					}
				}
			}
			else {
				memoryTupleIndex.at(minimumBlockIndex) = minimumTupleIndex + 1;
			}
		}
		//Next first tuple
		for (int i = 0; i < totalNoofTables1; i++) {
			if (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
				Relation * relation_pointer = memoryBlockIndex[i];
				if (memoryTupleIndex[i] < mem.getBlock(i)->getNumTuples()) {
					minimumTuple1 = mem.getBlock(i)->getTuple(memoryTupleIndex[i]);
					minimumBlockIndex1 = i;
					minimumTupleIndex1 = memoryTupleIndex[i];
					break;
				}
				else if (memoryTupleIndex[i] >= mem.getBlock(i)->getNumTuples()) {
					Relation* relationOfBlock = memoryBlockIndex[i];
					int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
					if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
						++noOfTables1Complete;
						continue;
					}
					else {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
						relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
						memoryTupleIndex.at(i) = 0;
						minimumTuple1 = mem.getBlock(i)->getTuple(memoryTupleIndex[i]);
						minimumBlockIndex1 = i;
						minimumTupleIndex1 = memoryTupleIndex[i];
						break;
					}
				}
			}

		}

		for (int i = subListsOfTable1.size(); i < subListsOfTable1.size() + subListsOfTable2.size(); i++) {
			if (memoryBlockIndex.find(i) != memoryBlockIndex.end() && mapOfRelationNamesWithBlocks[memoryBlockIndex[i]] != -1) {
				Relation * relation_pointer = memoryBlockIndex[i];
				if (memoryTupleIndex[i] < mem.getBlock(i)->getNumTuples()) {
					minimumTuple2 = mem.getBlock(i)->getTuple(memoryTupleIndex[i]);
					minimumBlockIndex2 = i;
					minimumTupleIndex2 = memoryTupleIndex[i];
					break;
				}
				else if (memoryTupleIndex[i] >= mem.getBlock(i)->getNumTuples()) {
					Relation* relationOfBlock = memoryBlockIndex[i];
					int diskBlockIndex = mapOfRelationNamesWithBlocks[relationOfBlock];
					if (relationOfBlock->getNumOfBlocks() <= diskBlockIndex + 1) {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = -1;
						++noOfTables2Complete;
						continue;
					}
					else {
						mapOfRelationNamesWithBlocks.at(relationOfBlock) = diskBlockIndex + 1;
						relationOfBlock->getBlock(mapOfRelationNamesWithBlocks[relationOfBlock], i);
						memoryTupleIndex.at(i) = 0;
						minimumTuple2 = mem.getBlock(i)->getTuple(memoryTupleIndex[i]);
						minimumBlockIndex2 = i;
						minimumTupleIndex2 = memoryTupleIndex[i];
						break;
					}
				}
			}
		}

	}
	if (!resultant_block_pointer->isEmpty()) {
		joinedTable->setBlock(relation_block_index++, mem.getMemorySize() - 1);
		resultant_block_pointer->clear();
	}
	return joinedTable;
}


bool deleteTable(string table_name, SchemaManager &schema_manager, vector<vector<JoinCondition*>> &listOfJoinConditions, MainMemory& mem, std::ofstream& fwriteExecute) {
	try {
		Relation *table_relation = schema_manager.getRelation(table_name);
		if (table_relation == NULL) {
			throw "Given relation with " + table_name + " Does not exist";
		}
		Schema schema = table_relation->getSchema();
		verifySchema(schema, listOfJoinConditions, table_name);
		Block *block_pointer = mem.getBlock(0);
		int numOfBlocks = table_relation->getNumOfBlocks();
		for (int i = 0; i < numOfBlocks; i++) {
			table_relation->getBlock(i, 0);
			vector<Tuple> listOfTuples = block_pointer->getTuples();
			vector<Tuple> listOfTuplesWhichPassedSearchCondition;
			vector<Tuple>::iterator itr;
			for (itr = listOfTuples.begin(); itr != listOfTuples.end(); itr++) {
				Tuple tuple = *itr;
				bool resultOfCheckingOnConditions = checkIfTupleSatisfiesConditions(tuple, schema, listOfJoinConditions);
				if (!resultOfCheckingOnConditions) {
					listOfTuplesWhichPassedSearchCondition.push_back(tuple);
				}
			}
			block_pointer->clear();
			for (itr = listOfTuplesWhichPassedSearchCondition.begin(); itr != listOfTuplesWhichPassedSearchCondition.end(); itr++) {
				block_pointer->appendTuple(*itr);
			}
			table_relation->setBlock(i, 0);

		}

	}
	catch (std::string s) {
		fwriteExecute << s<<"\n\n";
		return false;
	}
	return true;
}


Relation* createProduct(Relation* smallRelation, Relation* largeRelation, SchemaManager& schema_manager, MainMemory& mem, vector<vector<JoinCondition*>> &listOfJoinConditions, std::ofstream& fWriteExecute)
{
	Relation* table1 = smallRelation;
	Relation* table2 = largeRelation;
	int memBlockNumber;
	int numOfBlocksTable1 = table1->getNumOfBlocks();
	Block* outputBlock = mem.getBlock(NUM_OF_BLOCKS_IN_MEMORY - 1);
	outputBlock->clear();
	Schema schema1 = table1->getSchema();
	Schema schema2 = table2->getSchema();
	vector<string> fieldNames1 = schema1.getFieldNames();
	vector<string> fieldNames2 = schema2.getFieldNames();
	vector<string> mergedFieldNames;
	vector<FIELD_TYPE> fieldTypes1 = schema1.getFieldTypes();
	vector<FIELD_TYPE> fieldTypes2 = schema2.getFieldTypes();
	vector<FIELD_TYPE> mergedFieldTypes;

	mergedFieldNames = fieldNames1;
	mergedFieldNames.insert(mergedFieldNames.end(), fieldNames2.begin(), fieldNames2.end());
	mergedFieldTypes = fieldTypes1;
	mergedFieldTypes.insert(mergedFieldTypes.end(), fieldTypes2.begin(), fieldTypes2.end());

	Relation* mergedRelation = createTable(schema_manager, getIntermediateTableName(), mergedFieldNames, mergedFieldTypes, fWriteExecute);
	int blockCount = 0;
	for (int iterationCount = 0; iterationCount < numOfBlocksTable1 / (NUM_OF_BLOCKS_IN_MEMORY - 2); iterationCount++)
	{
		int blocksOfTable1InMemory = NUM_OF_BLOCKS_IN_MEMORY - 2;
		for (memBlockNumber = 0; memBlockNumber < blocksOfTable1InMemory; memBlockNumber++)
		{
			Block* memBlock = mem.getBlock(memBlockNumber);
			memBlock->clear();
			table1->getBlock(iterationCount*blocksOfTable1InMemory + memBlockNumber, memBlockNumber);
		}

		
		for (int j = 0; j < table2->getNumOfBlocks(); j++)
		{
			Block* block2 = mem.getBlock(NUM_OF_BLOCKS_IN_MEMORY - 2);
			block2->clear();
			table2->getBlock(j, NUM_OF_BLOCKS_IN_MEMORY - 2);
			for (int i = 0; i < blocksOfTable1InMemory; i++)
			{
				Block* block1 = mem.getBlock(i);
				for (int k = 0; k < block1->getNumTuples(); k++)
				{
					Tuple t1 = block1->getTuple(k);
					for (int l = 0; l < block2->getNumTuples(); l++)
					{
						Tuple t2 = block2->getTuple(l);
						vector<Field> fieldValues1, fieldValues2, mergedFieldValues;
						int m;
						for (m = 0; m < t1.getNumOfFields(); m++)
							fieldValues1.push_back(t1.getField(m));
						for (m = 0; m < t2.getNumOfFields(); m++)
							fieldValues2.push_back(t2.getField(m));
						mergedFieldValues = fieldValues1;
						mergedFieldValues.insert(mergedFieldValues.end(), fieldValues2.begin(), fieldValues2.end());
						Tuple mergedTuple = mergedRelation->createTuple();
						for (int iter = 0; iter < mergedFieldValues.size(); iter++)
						{
							if (mergedFieldTypes[iter] == STR20)
								mergedTuple.setField(iter, *(mergedFieldValues[iter].str));
							else if (mergedFieldTypes[iter] == INT)
								mergedTuple.setField(iter, mergedFieldValues[iter].integer);
						}
						if (outputBlock->isFull())
						{
							mergedRelation->setBlock(blockCount++, NUM_OF_BLOCKS_IN_MEMORY - 1);
							outputBlock->clear();
						}
						bool resultOfCheckingOnConditions = checkIfTupleSatisfiesConditions(mergedTuple, mergedRelation->getSchema(), listOfJoinConditions);
						if (resultOfCheckingOnConditions)
							outputBlock->appendTuple(mergedTuple);
					}
				}
			}
		}
	}
	if ((numOfBlocksTable1 % (NUM_OF_BLOCKS_IN_MEMORY - 2)) != 0)
	{
		int blocksOfTable1InMemory = numOfBlocksTable1 % (NUM_OF_BLOCKS_IN_MEMORY - 2);
		for (memBlockNumber = 0; memBlockNumber < blocksOfTable1InMemory; memBlockNumber++)
		{
			Block* memBlock = mem.getBlock(memBlockNumber);
			memBlock->clear();
			int relationBlockIndex = (numOfBlocksTable1 / (NUM_OF_BLOCKS_IN_MEMORY - 2))*(NUM_OF_BLOCKS_IN_MEMORY - 2) + memBlockNumber;
			table1->getBlock(relationBlockIndex, memBlockNumber);
		}

		
		for (int j = 0; j < table2->getNumOfBlocks(); j++)
		{
			Block* block2 = mem.getBlock(NUM_OF_BLOCKS_IN_MEMORY - 2);
			block2->clear();
			table2->getBlock(j, NUM_OF_BLOCKS_IN_MEMORY - 2);
			for (int i = 0; i < blocksOfTable1InMemory; i++)
			{
				Block* block1 = mem.getBlock(i);
				for (int k = 0; k < block1->getNumTuples(); k++)
				{
					Tuple t1 = block1->getTuple(k);
					for (int l = 0; l < block2->getNumTuples(); l++)
					{
						Tuple t2 = block2->getTuple(l);
						vector<Field> fieldValues1, fieldValues2, mergedFieldValues;
						int m;
						for (m = 0; m < t1.getNumOfFields(); m++)
							fieldValues1.push_back(t1.getField(m));
						for (m = 0; m < t2.getNumOfFields(); m++)
							fieldValues2.push_back(t2.getField(m));
						mergedFieldValues = fieldValues1;
						mergedFieldValues.insert(mergedFieldValues.end(), fieldValues2.begin(), fieldValues2.end());
						Tuple mergedTuple = mergedRelation->createTuple();
						for (int iter = 0; iter < mergedFieldValues.size(); iter++)
						{
							if (mergedFieldTypes[iter] == STR20)
								mergedTuple.setField(iter, *(mergedFieldValues[iter].str));
							else if (mergedFieldTypes[iter] == INT)
								mergedTuple.setField(iter, mergedFieldValues[iter].integer);
						}
						if (outputBlock->isFull())
						{
							mergedRelation->setBlock(blockCount++, NUM_OF_BLOCKS_IN_MEMORY - 1);
							outputBlock->clear();
						}
						bool resultOfCheckingOnConditions = checkIfTupleSatisfiesConditions(mergedTuple, mergedRelation->getSchema(), listOfJoinConditions);
						if (resultOfCheckingOnConditions)
							outputBlock->appendTuple(mergedTuple);
					}
				}
			}
		}
	}
	if (!(outputBlock->isEmpty()))
		mergedRelation->setBlock(blockCount, NUM_OF_BLOCKS_IN_MEMORY - 1);
	return mergedRelation;
}


Relation* cartesianProductOnePass(vector<Relation*>&  tables, SchemaManager& schema_manager, MainMemory& mem, vector<vector<JoinCondition*>> &listOfJoinConditions, std::ofstream& fWriteExecute)
{
	return createProduct(tables[0], tables[1], schema_manager, mem, listOfJoinConditions, fWriteExecute);
}

Relation* cartesianProductOnePass(vector<string>&  tablesNames, SchemaManager& schema_manager, MainMemory& mem, vector<vector<JoinCondition*>> &listOfJoinConditions, std::ofstream& fwriteExecute)
{
	vector<Relation*> relationPtrs;
	try
	{
		for (vector<string>::iterator tableIter = tablesNames.begin(); tableIter != tablesNames.end(); tableIter++)
		{
			Relation* ptr = schema_manager.getRelation((*tableIter));
			if (!ptr)
				throw "Given relation with " + (*tableIter) + " Does not exist";
			relationPtrs.push_back(ptr);
		}
		return cartesianProductOnePass(relationPtrs, schema_manager, mem, listOfJoinConditions, fwriteExecute);
	}
	catch (std::string s) {
		fwriteExecute << s<<"\n\n";
		return NULL;
	}
}

Relation * onePassOrdering(Relation * table_name, MainMemory& mem, SchemaManager& schema_manager, vector<OperandOperator*>& attributesList, std::ofstream& fwriteExecute) {
	if (table_name->getNumOfBlocks() > mem.getMemorySize() - 1) {
		fwriteExecute << "Error in doing One pass as table blocks do not fit into memory\n\n";
		return NULL;
	}
	Relation *sortedTable = schema_manager.createRelation(getIntermediateTableName(), table_name->getSchema());
	if (sortedTable == NULL) {
		fwriteExecute << "Error in creating of resultant table\n\n";
			return NULL;
	}
	table_name->getBlocks(0, 0, table_name->getNumOfBlocks());
	vector<Tuple> listOfTuples;
	for (int i = 0; i < table_name->getNumOfBlocks(); i++) {
		Block * block_pointer = mem.getBlock(i);
		vector<Tuple> listOfTuplesOfBlock = block_pointer->getTuples();
		listOfTuples.insert(listOfTuples.end(), listOfTuplesOfBlock.begin(), listOfTuplesOfBlock.end());
	}
	mergeSortTuples(listOfTuples, attributesList, table_name->getSchema());
	Block * resultant_block = mem.getBlock(mem.getMemorySize() - 1);
	resultant_block->clear();
	vector<Tuple>::iterator itr;
	int relation_block_index = 0;
	for (itr = listOfTuples.begin(); itr != listOfTuples.end(); itr++) {
		if (resultant_block->isFull()) {
			sortedTable->setBlock(relation_block_index++, mem.getMemorySize() - 1);
			resultant_block->clear();
		}
		resultant_block->appendTuple(*itr);
	}
	if (!resultant_block->isEmpty()) {
		sortedTable->setBlock(relation_block_index++, mem.getMemorySize() - 1);
		resultant_block->clear();
	}
	return sortedTable;
}

Relation * onePassDistinct(Relation * table_name, MainMemory& mem, SchemaManager& schema_manager, vector<OperandOperator*>& attributesList, std::ofstream& fwriteExecute) {
	if (table_name->getNumOfBlocks() > mem.getMemorySize() - 1) {
		fwriteExecute << "Error in doing One pass as table blocks do not fit into memory\n\n";
		return NULL;
	}
	Relation *distinctTable = schema_manager.createRelation(getIntermediateTableName(), table_name->getSchema());
	if (distinctTable == NULL) {
		fwriteExecute << "Error in creating of resultant table\n\n";
		return NULL;
	}
	table_name->getBlocks(0, 0, table_name->getNumOfBlocks());
	vector<Tuple> listOfTuples;
	for (int i = 0; i < table_name->getNumOfBlocks(); i++) {
		Block * block_pointer = mem.getBlock(i);
		vector<Tuple> listOfTuplesOfBlock = block_pointer->getTuples();
		listOfTuples.insert(listOfTuples.end(), listOfTuplesOfBlock.begin(), listOfTuplesOfBlock.end());
	}
	vector<string> fieldnames;
	vector<OperandOperator*>::iterator iteratorForOperandOperator;
	for (iteratorForOperandOperator = attributesList.begin(); iteratorForOperandOperator != attributesList.end(); iteratorForOperandOperator++) {
		fieldnames.push_back((*iteratorForOperandOperator)->getName());
	}
	mergeSortTuples(listOfTuples, attributesList, table_name->getSchema());
	Block * resultant_block = mem.getBlock(mem.getMemorySize() - 1);
	resultant_block->clear();
	vector<Tuple>::iterator itr;
	int relation_block_index = 0;
	itr = listOfTuples.begin();
	while(itr != listOfTuples.end()){
		if (resultant_block->isFull()) {
			distinctTable->setBlock(relation_block_index++, mem.getMemorySize() - 1);
			resultant_block->clear();
		}
		resultant_block->appendTuple(*itr);
		Tuple distinctTuple = *itr;
		itr++;
		while (itr != listOfTuples.end()) {
			if (compareEquality(distinctTuple, *itr, table_name->getSchema(), fieldnames) == true) {
				itr++;
			}
			else {
				break;
			}
		}
	}
	if (!resultant_block->isEmpty()) {
		distinctTable->setBlock(relation_block_index++, mem.getMemorySize() - 1);
		resultant_block->clear();
	}
	return distinctTable;
}