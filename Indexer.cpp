#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <memory>
using namespace std;

template <typename T>
T getValue(const map<string, T> &m, const string &key, T defaultVal)
{
    auto it = m.find(key);
    return (it != m.end()) ? it->second : defaultVal;
}

class Buffer
{
private:
    unique_ptr<char[]> buffer; // use unique_ptr for automatic memory management
    int size;
    ifstream file;
    int bytesRead; // to store the number of bytes read in a chunk
public:
    Buffer(string filename, int bufferSize)
    {
        size = bufferSize;
        buffer = unique_ptr<char[]>(new char[size]); // allocate buffer memory
        file.open(filename);
        if (!file.is_open())
        {
            throw runtime_error("Error opening file: " + filename);
        }
    }
    ~Buffer()
    {
        if (file.is_open())
        {
            file.close();
        }
    }
    bool readChunk()
    {
        file.read(buffer.get(), size);
        bytesRead = file.gcount(); // store actual bytes read
        return bytesRead > 0;
    }
    int getBytesRead() const
    {
        return bytesRead; // return the number of bytes read in the last chunk
    }
    char *getBuffer() const
    {
        return buffer.get(); // return raw pointer to the buffer
    }
};

class Tokenizer
{
private:
    string leftover; // to store any leftover word from the previous chunk
public:
    Tokenizer() : leftover("") {}
    vector<string> tokenize(const char *buffer, int bytesRead)
    {
        vector<string> tokens;
        string currentWord = "";
        for (char c : leftover)
            currentWord += tolower(c); // add leftover characters to the current word
        for (int i = 0; i < bytesRead; i++)
        {
            char c = tolower(buffer[i]);
            if (isalnum(c))
            {
                currentWord += c;
            }
            else
            {
                if (!currentWord.empty())
                {
                    tokens.push_back(currentWord);
                    currentWord = "";
                }
            }
        }
        leftover = currentWord; // store any leftover word for the next chunk
        return tokens;
    }

    vector<string> flush()
    {
        vector<string> pendingTokens;
        if (!leftover.empty())
        {
            pendingTokens.push_back(leftover);
            leftover = "";
        }
        return pendingTokens;
    }
};

class VersionIndex
{
private:
    map<string, map<string, int>> index; // version -> (word -> count)
public:
    void addTokens(const string &version, const vector<string> &tokens)
    {
        for (const string &token : tokens)
        {
            index[version][token]++;
        }
    }
    const map<string, int> &getWordCounts(const string &version) const
    {
        if (index.find(version) == index.end())
        {
            throw runtime_error("Version not found: " + version);
        }
        return index.at(version);
    }
    const map<string, map<string, int>> &getIndex() const
    {
        return index;
    }
    int getWordCounts(const string &version, const string &word) const
    {
        if (index.find(version) == index.end())
        {
            throw runtime_error("Version not found: " + version);
        }
        return getValue(index.at(version), word, 0); // return count of the word or 0 if not found
    }
};

class Query
{
protected:
    VersionIndex &versionIndex; // all queries will operate on the same version index
public:
    Query(VersionIndex &vi) : versionIndex(vi) {}
    virtual void compute() = 0; // pure virtual function to be implemented by derived classes
    virtual ~Query() {}
};

class WordCountQuery : public Query
{
private:
    string version;
    string word;

public:
    WordCountQuery(VersionIndex &vi, const string &ver, const string &w) : Query(vi), version(ver), word(w) {}
    void compute() override
    {
        try
        {
            int count = versionIndex.getWordCounts(version, word);
            cout << "The count of the word '" << word << "' in version " << version << " is: " << count << endl;
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }
};

class DifferenceQuery : public Query
{
private:
    string version1;
    string version2;
    string word;

public:
    DifferenceQuery(VersionIndex &vi, const string &ver1, const string &ver2, const string &w) : Query(vi), version1(ver1), version2(ver2), word(w) {}
    void compute() override
    {
        try
        {
            int count1 = versionIndex.getWordCounts(version1, word);
            int count2 = versionIndex.getWordCounts(version2, word);
            cout << "The difference in count of the word '" << word << "' between version " << version1 << " and version " << version2 << " is: " << (count1 - count2) << endl;
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }
};

class TopKQuery : public Query
{
private:
    string version;
    int k;

public:
    TopKQuery(VersionIndex &vi, const string &ver, int topK) : Query(vi), version(ver), k(topK) {}
    void compute() override
    {
        try
        {
            const map<string, int> &wordCounts = versionIndex.getWordCounts(version);
            vector<pair<string, int>> wordList(wordCounts.begin(), wordCounts.end());

            // Sort by count in descending order
            sort(wordList.begin(), wordList.end(), [](const pair<string, int> &a, const pair<string, int> &b)
                 {
                     return a.second > b.second; // sort in descending order
                 });

            cout << "Top " << k << " words in version " << version << ":" << endl;
            for (int i = 0; i < min(k, (int)wordList.size()); i++)
            {
                cout << wordList[i].first << " (count: " << wordList[i].second << ")" << endl;
            }
        }
        catch (const exception &e)
        {
            cerr << "Error: " << e.what() << endl;
        }
    }
};

int main(int argc, char *argv[])
{

    try
    {

        auto start = chrono::high_resolution_clock::now(); // Start measuring time

        // Parse command line arguments for file name and buffer size
        string file1, file2, version1, version2, queryType, word;
        int bufferKB = 512, k = 0; // default bufferKB = 512 and k value = 0 for topk query

        for (int i = 1; i < argc; i++)
        {
            string arg = argv[i];
            if (arg == "--file" && i + 1 < argc)
            {
                string filename = argv[++i];
                if (filename.substr(0, 2) == "--")
                {
                    throw runtime_error("Expected file path after --file, but got another argument: " + filename);
                }
                file1 = filename; // if --file is provided, use it for file1
            }
            else if (arg == "--version" && i + 1 < argc)
            {
                string ver = argv[++i];
                if (ver.substr(0, 2) == "--")
                {
                    throw runtime_error("Expected version name after --version, but got another argument: " + ver);
                }
                version1 = ver; // if --version is provided, use it for version1
            }
            else if (arg == "--file1" && i + 1 < argc)
            {
                string filename = argv[++i];
                if (filename.substr(0, 2) == "--")
                {
                    throw runtime_error("Expected file path after --file1, but got another argument: " + filename);
                }
                file1 = filename;
            }
            else if (arg == "--file2" && i + 1 < argc)
            {
                string filename = argv[++i];
                if (filename.substr(0, 2) == "--")
                {
                    throw runtime_error("Expected file path after --file2, but got another argument: " + filename);
                }
                file2 = filename;
            }
            else if (arg == "--version1" && i + 1 < argc)
            {
                string ver = argv[++i];
                if (ver.substr(0, 2) == "--")
                {
                    throw runtime_error("Expected version name after --version1, but got another argument: " + ver);
                }
                version1 = ver;
            }
            else if (arg == "--version2" && i + 1 < argc)
            {
                string ver = argv[++i];
                if (ver.substr(0, 2) == "--")
                {
                    throw runtime_error("Expected version name after --version2, but got another argument: " + ver);
                }
                version2 = ver;
            }
            else if (arg == "--query" && i + 1 < argc)
            {
                queryType = argv[++i];
            }
            else if (arg == "--word" && i + 1 < argc)
            {
                word = argv[++i];
            }
            else if (arg == "--buffer" && i + 1 < argc)
            {
                try
                {
                    bufferKB = stoi(argv[++i]);
                }
                catch (...)
                {
                    throw runtime_error("Invalid buffer size. Use --buffer <kb> with a valid integer value.");
                }
            }
            else if (arg == "--top" && i + 1 < argc)
            {
                try
                {
                    k = stoi(argv[++i]);
                }
                catch (...)
                {
                    throw runtime_error("Invalid value for top k. Use --top <k> with a valid integer value.");
                }
            }
        }

        if (file1.empty())
        {
            throw runtime_error("No input file provided. Use --file <path> or --file1 <path>.");
        }
        if (version1.empty())
        {
            throw runtime_error("No version provided for the first file. Use --version <version> or --version1 <version>.");
        }
        if (queryType.empty())
        {
            throw runtime_error("No query type provided. Use --query <query_type>.");
        }
        if ((queryType == "word" || queryType == "diff") && word.empty())
        {
            throw runtime_error("No word provided for wordcount or difference query. Use --word <word>.");
        }
        if (queryType == "diff" && (version2.empty() || file2.empty()))
        {
            throw runtime_error("For difference query, both version2 and file2 must be provided. Use --version2 <version> and --file2 <path >.");
        }
        if (queryType == "top" && k <= 0)
        {
            throw runtime_error("For topk query, a positive integer must be provided for k. Use --top <k>.");
        }
        if (bufferKB < 256 || bufferKB > 1024)
        {
            throw runtime_error("Buffer size must be between 256 and 1024 Kilobytes. Use --buffer <kb>.");
        }
        int bufferSize = bufferKB * 1024; // convert from KB to bytes

        VersionIndex versionIndex;
        // Process first file and build index for version1
        Buffer buffer1(file1, bufferSize);
        Tokenizer tokenizer1;
        Tokenizer tokenizer2; // declare tokenizer2 here to be used in both diff and topk queries
        while (buffer1.readChunk())
        {
            vector<string> tokens = tokenizer1.tokenize(buffer1.getBuffer(), buffer1.getBytesRead());
            versionIndex.addTokens(version1, tokens);
        }
        unique_ptr<Query> query; // declare query pointer here to be used in both diff and topk queries
        if (queryType == "word")
        {
            query = make_unique<WordCountQuery>(versionIndex, version1, word);
        }
        else if (queryType == "top")
        {
            query = make_unique<TopKQuery>(versionIndex, version1, k);
        }
        else if (queryType == "diff")
        {
            query = make_unique<DifferenceQuery>(versionIndex, version1, version2, word);
        }
        // If difference query, process second file and build index for version2
        if (queryType == "diff")
        {
            Buffer buffer2(file2, bufferSize);
            while (buffer2.readChunk())
            {
                vector<string> tokens = tokenizer2.tokenize(buffer2.getBuffer(), buffer2.getBytesRead());
                versionIndex.addTokens(version2, tokens);
            }
        }
        // Flush any remaining tokens in the buffers
        vector<string> pendingTokens1 = tokenizer1.flush();
        if (!pendingTokens1.empty())
        {
            versionIndex.addTokens(version1, pendingTokens1);
        }
        if (queryType == "diff")
        {
            vector<string> pendingTokens2 = tokenizer2.flush();
            if (!pendingTokens2.empty())
            {
                versionIndex.addTokens(version2, pendingTokens2);
            }
        }
        if (!query)
        {
            throw runtime_error("Invalid query type. Use --query <word|diff|top>.");
        }
        if (queryType == "diff")
        {
            cout << "Version names: " << version1 << " and " << version2 << endl;
        }
        else
        {
            cout << "Version name: " << version1 << endl;
        }
        query->compute(); // Execute the query

        auto end = chrono::high_resolution_clock::now(); // End measuring time
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Buffer size: " << bufferSize / 1024 << " KB" << endl;
        cout << endl;
        cout << "Execution time: " << fixed << setprecision(3) << duration / 1000.0 << " s" << endl;
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
    }
}
