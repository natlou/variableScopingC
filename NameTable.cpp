// I will be implementing a hashtable to store scopes with it's declarations , 
#include "NameTable.h"
#include <string>
#include <vector>
#include <list>
using namespace std;

// hash function turns strings into index


struct Item // Data type BUCKET will hold
{
    Item(string id, int lineNum, int scopeIndex) : m_id(id), m_lineNum(lineNum), m_scopeIndex(scopeIndex)
    {
        next = nullptr;
    }
    string m_id;
    int m_lineNum;
    int m_scopeIndex;
    Item* next; // next item in the bucket if we have a collision 
};

class HashTable
{
public:
    void insert(const string id, int lineNum, int scopeIndex, Item**& update);
    bool search(const string id, int& lineNum, int& scopeIndex);
private:

    struct Bucket
    {
        Bucket() { item = nullptr; }
        Item* item;
    };

    static const int table_size = 19991; // 19991 is the largest prime number that is less than 20000, which is why we (I) chose it 
    size_t hash_function(const string& id) const;
    Bucket buckets[table_size];
};

size_t HashTable::hash_function(const string& id) const
{
    size_t hash = 7;
    size_t index;

    for (size_t i = 0; i < id.length(); i++)
    {
        hash += id[i];
        hash *= 2;
    }

    index = hash % table_size;

    return (index);
}

bool HashTable::search(const string id, int& lineNum, int& scopeIndex)
{
    
    int index = hash_function(id);
    Item* Ptr = buckets[index].item;
    lineNum = -1;

    while (Ptr != nullptr)
    {
        if (Ptr->m_id == id)
        {
            lineNum = Ptr->m_lineNum; 
            scopeIndex = Ptr->m_scopeIndex;
        }
        Ptr = Ptr->next;
    }

    if (lineNum == -1) return false;

    return true;
}

void HashTable::insert(const string id, int lineNum, int scopeIndex, Item**& update)
{
    int index = hash_function(id);
    Item* Ptr = buckets[index].item;

    if (Ptr == nullptr)   //no item in the bucket, dynamic allocation
    {
        buckets[index].item = new Item(id, lineNum, scopeIndex);
        update = &buckets[index].item;
        return;
    }

    Item* temp = nullptr;
    while (Ptr != nullptr)   //loop until empty spot found
    {
        temp = Ptr;
        Ptr = Ptr->next;
    }

    temp->next = new Item(id, lineNum, scopeIndex);
    update = &temp->next;
}



class NameTableImpl
{
public:
    NameTableImpl();
    ~NameTableImpl();
    void enterScope();
    bool exitScope();
    bool declare(const string& id, int lineNum);
    int find(const string& id) const;
private:
    HashTable* hash_table; // there is only one table for all of the scopes, so we only need to generate one hash table 
    vector<Item**> buckets; // basically, this stores everything, just a single hashtable. It maps scopes to buckets. Double asterisk since it's being mapped and we need to keep track so no dangling pointers
    int current_scope; // current scope to keep track
};

NameTableImpl::NameTableImpl()
{
    current_scope = 0; 
    hash_table = new HashTable; // creates a hash table for the first (and only) time very efficient !
}

NameTableImpl::~NameTableImpl()
{
    // we must delete all pointers 

    size_t i = buckets.size();

    while (i > 0)
    {
        i--;
        if (buckets[i] != nullptr) delete (*buckets[i]);
    }

    delete hash_table;
}

void NameTableImpl::enterScope()
{
    buckets.push_back(nullptr); // add a new scope / bucket (seperate with nullptrs)
    current_scope++;
}


bool NameTableImpl::exitScope()
{
    size_t i = buckets.size();

    while (i > 0)
    {
        i--;
        if (buckets[i] == nullptr) break; // that means we are in a new scope

        delete (*buckets[i]); // delete since we new'd it 
        (*buckets[i]) = nullptr; // null allows us to recognize empty spot
        buckets.pop_back();
    }

    if (buckets.empty()) return false;

    buckets.pop_back();
    current_scope--;

    return true;
}

bool NameTableImpl::declare(const string& id, int lineNum)
{
    if (id.empty()) return false;

    int scopeIndex; 
    int line;
    if (hash_table->search(id, line, scopeIndex) && scopeIndex == current_scope) return false; // check it has already been declared in scope

    Item** Ptr;
    hash_table->insert(id, lineNum, current_scope, Ptr);
    buckets.push_back(Ptr);
    return true;
}

int NameTableImpl::find(const string& id) const
{
    if (id.empty()) return -1;

    int line;
    int scopeIndex;

    hash_table->search(id, line, scopeIndex);

    return line;
}


//*********** NameTable functions **************

// For the most part, these functions simply delegate to NameTableImpl's
// functions.

NameTable::NameTable()
{
    m_impl = new NameTableImpl;
}

NameTable::~NameTable()
{
    delete m_impl;
}

void NameTable::enterScope()
{
    m_impl->enterScope();
}

bool NameTable::exitScope()
{
    return m_impl->exitScope();
}

bool NameTable::declare(const string& id, int lineNum)
{
    return m_impl->declare(id, lineNum);
}

int NameTable::find(const string& id) const
{
    return m_impl->find(id);
}