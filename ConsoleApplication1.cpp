// NameTable tester
//
// The file commands.txt should contain a sequence of lines of the form
//   {                    which requests a call to enterScope
//   }                    which requests a call to exitScope
//   identifier number    which requests a call to declare(identifier,number)
//   identifier           which requests a call to find(identifier)

#include "NameTable.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <list>
#include <stack>
using namespace std;

const char* COMMAND_FILE_NAME = "C:/Users/natlo/Desktop/commands.txt";


class SlowNameTable
{
public:
    SlowNameTable() { m_scope = 0; hash_table = new HashTable; }
    ~SlowNameTable();
    void enterScope();
    bool exitScope();
    bool declare(const string& id, int lineNum);
    int find(const string& id) const;
private:
    HashTable* hash_table; // there is only one table for all of the scopes, so we only need to generate one hash table 
    vector<Item**> buckets; // we use two pointers to prevent a segmentation fault in assignment, as each Bucket containers Item pointers (scope)
    int m_scope;
};

struct Command
{
    static Command* create(string line, int lineno);
    Command(string line, int lineno) : m_line(line), m_lineno(lineno) {}
    virtual ~Command() {}
    virtual void execute(NameTable& nt) const = 0;
    virtual bool executeAndCheck(NameTable& nt, SlowNameTable& snt) const = 0;
    string m_line;
    int m_lineno;
};

void extractCommands(istream& dataf, vector<Command*>& commands);
string testCorrectness(const vector<Command*>& commands);
void testPerformance(const vector<Command*>& commands);

int main()
{
    vector<Command*> commands;

    // Basic correctness test

    // Notice that this test string contains an excess "}" command at the
    // end.  That tests whether you correctly return false if you call
    // exitScope more times than you call enterScope.

    istringstream basicf(
        "alpha 1\n"
        "beta 2\n"
        "p1 3\n"
        "alpha\n"
        "p2 4\n"
        "beta\n"
        "p3 5\n"
        "gamma\n"
        "f 6\n"
        "{\n"
        "beta 7\n"
        "gamma 8\n"
        "alpha\n"
        "beta\n"
        "gamma\n"
        "{\n"
        "alpha 13\n"
        "beta 14\n"
        "beta 15\n"
        "alpha\n"
        "}\n"
        "alpha\n"
        "beta\n"
        "{\n"
        "beta 21\n"
        "beta\n"
        "}\n"
        "}\n"
        "p4 25\n"
        "alpha\n"
        "p5 26\n"
        "beta\n"
        "p6 27\n"
        "gamma\n"
        "main 28\n"
        "{\n"
        "beta 29\n"
        "beta\n"
        "f\n"
        "}\n"
        "}\n"
    );
    extractCommands(basicf, commands);

    cout << "Basic correctness test: " << flush;
    cout << testCorrectness(commands) << endl;

    for (size_t k = 0; k < commands.size(); k++)
        delete commands[k];
    commands.clear();

    // Thorough correctness and performance tests

    ifstream thoroughf(COMMAND_FILE_NAME);
    if (!thoroughf)
    {
        cout << "Cannot open " << COMMAND_FILE_NAME
            << ", so cannot do thorough correctness or performance tests!"
            << endl;
        return 1;
    }
    extractCommands(thoroughf, commands);

    cout << "Thorough correctness test: " << flush;
    cout << testCorrectness(commands) << endl;

    cout << "Performance test on " << commands.size() << " commands: " << flush;
    testPerformance(commands);

    for (size_t k = 0; k < commands.size(); k++)
        delete commands[k];
}

struct EnterScopeCmd : public Command
{
    EnterScopeCmd(string line, int lineno)
        : Command(line, lineno)
    {}
    virtual void execute(NameTable& nt) const
    {
        nt.enterScope();
    }
    virtual bool executeAndCheck(NameTable& nt, SlowNameTable& snt) const
    {
        nt.enterScope();
        snt.enterScope();
        return true;
    }
};

struct ExitScopeCmd : public Command
{
    ExitScopeCmd(string line, int lineno)
        : Command(line, lineno)
    {}
    virtual void execute(NameTable& nt) const
    {
        nt.exitScope();
    }
    virtual bool executeAndCheck(NameTable& nt, SlowNameTable& snt) const
    {
        return nt.exitScope() == snt.exitScope();
    }
};

struct DeclareCmd : public Command
{
    DeclareCmd(string id, int lineNum, string line, int lineno)
        : Command(line, lineno), m_id(id), m_lineNum(lineNum)
    {}
    virtual void execute(NameTable& nt) const
    {
        nt.declare(m_id, m_lineNum);
    }
    virtual bool executeAndCheck(NameTable& nt, SlowNameTable& snt) const
    {
        return nt.declare(m_id, m_lineNum) == snt.declare(m_id, m_lineNum);
    }
    string m_id;
    int m_lineNum;
};

struct FindCmd : public Command
{
    FindCmd(string id, string line, int lineno)
        : Command(line, lineno), m_id(id)
    {}
    virtual void execute(NameTable& nt) const
    {
        nt.find(m_id);
    }
    virtual bool executeAndCheck(NameTable& nt, SlowNameTable& snt) const
    {
        return nt.find(m_id) == snt.find(m_id);
    }
    string m_id;
};

Command* Command::create(string line, int lineno)
{
    istringstream iss(line);
    string field1;
    if (!(iss >> field1))
        return nullptr;
    if (field1 == "{")
        return new EnterScopeCmd(line, lineno);
    if (field1 == "}")
        return new ExitScopeCmd(line, lineno);
    int field2;
    if (!(iss >> field2))
        return new FindCmd(field1, line, lineno);
    return new DeclareCmd(field1, field2, line, lineno);
}

void extractCommands(istream& dataf, vector<Command*>& commands)
{
    string commandLine;
    int commandNumber = 0;
    while (getline(dataf, commandLine))
    {
        commandNumber++;
        Command* cmd = Command::create(commandLine, commandNumber);
        if (cmd != nullptr)
            commands.push_back(cmd);
    }
}

string testCorrectness(const vector<Command*>& commands)
{
    NameTable nt;
    SlowNameTable snt;
    for (size_t k = 0; k < commands.size(); k++)
    {
        // Check if command agrees with our behavior

        if (!commands[k]->executeAndCheck(nt, snt))
        {
            ostringstream msg;
            msg << "*** FAILED *** line " << commands[k]->m_lineno
                << ": \"" << commands[k]->m_line << "\"";
            return msg.str();
        }
    }
    return "Passed";
}

//========================================================================
// Timer t;                 // create a timer and start it
// t.start();               // (re)start the timer
// double d = t.elapsed();  // milliseconds since timer was last started
//========================================================================

#include <chrono>

class Timer
{
public:
    Timer()
    {
        start();
    }
    void start()
    {
        m_time = std::chrono::high_resolution_clock::now();
    }
    double elapsed() const
    {
        std::chrono::duration<double, std::milli> diff =
            std::chrono::high_resolution_clock::now() - m_time;
        return diff.count();
    }
private:
    std::chrono::high_resolution_clock::time_point m_time;
};

void testPerformance(const vector<Command*>& commands)
{
    double endConstruction;
    double endCommands;

    Timer timer;
    {
        NameTable nt;

        endConstruction = timer.elapsed();

        for (size_t k = 0; k < commands.size(); k++)
            commands[k]->execute(nt);

        endCommands = timer.elapsed();
    }

    double end = timer.elapsed();

    cout << end << " milliseconds." << endl
        << "   Construction: " << endConstruction << " msec." << endl
        << "       Commands: " << (endCommands - endConstruction) << " msec." << endl
        << "    Destruction: " << (end - endCommands) << " msec." << endl;
}

SlowNameTable::~SlowNameTable()
{
    //need to free any leftover allocated Info memory and
    //free the allocated HashTable
    size_t i = buckets.size();

    while (i > 0)
    {
        i--;
        if (buckets[i] != nullptr) delete (*buckets[i]);
    }
    delete hash_table;
}

void SlowNameTable::enterScope()
{
    buckets.push_back(nullptr); // add a new scope / bucket 
    m_scope++;
}


bool SlowNameTable::exitScope()
{
    size_t i = buckets.size();

    while (i > 0)
    {
        i--;
        if (buckets[i] == nullptr) // start of new scope found
            break;

        delete (*buckets[i]); // free memory and set pointer to null
        (*buckets[i]) = nullptr; //null allows us to recognize empty spot
        buckets.pop_back();
    }

    if (buckets.empty())  // unmatched }
        return false;

    buckets.pop_back();
    m_scope--;
    return true;
}

bool SlowNameTable::declare(const string& id, int lineNum)
{
    if (id.empty())
        return false;

    int scopeNum, line;
    if (hash_table->search(id, line, scopeNum) && scopeNum == m_scope)
        return false;   //found in same scope

    Item** p;
    hash_table->insert(id, lineNum, m_scope, p);
    buckets.push_back(p);
    return true;
}

int SlowNameTable::find(const string& id) const
{
    if (id.empty())
        return -1;

    int returnLine;
    int scopeNum;

    hash_table->search(id, returnLine, scopeNum);
    // if true, output returnLine; if false, output -1
    return returnLine;
}