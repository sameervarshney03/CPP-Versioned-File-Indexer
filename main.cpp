#include <bits/stdc++.h>
using namespace std;
using namespace std::chrono;

template<typename T>
void printTopK(const vector<pair<string,T>> &v){
    for(const auto &p : v){
        cout<<p.first<<" -> "<<p.second<<endl;
    }
}

class BufferedReader{
    ifstream file;
    size_t bufferSize;
    vector<char> buffer;

public:
    BufferedReader(string path, size_t kb){
        if(kb < 256 || kb > 1024)
            throw runtime_error("Buffer size must be between 256KB and 1024KB");
            
        bufferSize = kb*1024;

        file.open(path);
        if(!file)
            throw runtime_error("Cannot open file: " + path);

        buffer.resize(bufferSize);
    }

    bool readChunk(string &out){
        if(!file.good()) return false;

        file.read(buffer.data(),bufferSize);
        size_t readBytes=file.gcount();

        if(readBytes==0) return false;

        out.assign(buffer.data(),readBytes);
        return true;
    }
};

class Tokenizer{
    string leftover;

public:
    vector<string> tokenize(string chunk){
        vector<string> tokens;
        string current = leftover;

        for(char c:chunk){
            if(isalnum(static_cast<unsigned char>(c)))
                current+=tolower(static_cast<unsigned char>(c));
            else{
                if(!current.empty()){
                    tokens.push_back(current);
                    current.clear();
                }
            }
        }

        leftover=current;
        return tokens;
    }

    string flush(){
        string t=leftover;
        leftover.clear();
        return t;
    }
};

class VersionedIndex{
    unordered_map<string, unordered_map<string,int>> versions;

public:

    void buildIndex(string version,string path,int bufferKB){
        BufferedReader reader(path,bufferKB);
        Tokenizer tokenizer;

        unordered_map<string,int> freq;
        string chunk;

        while(reader.readChunk(chunk)){
            auto tokens=tokenizer.tokenize(chunk);
            for(auto &w:tokens)
                freq[w]++;
        }

        string last=tokenizer.flush();
        if(!last.empty()) freq[last]++;

        versions[version]=std::move(freq);
    }

    int wordCount(string version, string word) const {
        string lower_word = word;
        transform(lower_word.begin(), lower_word.end(), lower_word.begin(), 
                  [](unsigned char c){ return tolower(c); });

        auto it = versions.find(version);
        if(it == versions.end())
            throw runtime_error("Version not found: " + version);

        auto w_it = it->second.find(lower_word);
        if(w_it == it->second.end()) 
            return 0;

        return w_it->second;
    }

    int diff(string v1, string v2, string word) const {
        if(!versions.count(v1) || !versions.count(v2))
            throw runtime_error("Version not found in diff");

        return wordCount(v1, word) - wordCount(v2, word);
    }

    vector<pair<string,int>> topK(string version, int k) const {
        auto it = versions.find(version);
        if(it == versions.end())
            throw runtime_error("Version not found: " + version);

        if (k <= 0) throw runtime_error("K must be positive");

        // We use a min-heap to keep at most K top elements.
        // A candidate is 'worse' (closer to being popped) if it has a smaller frequency,
        // or a larger word alphabetically when frequencies are equal.
        // std::priority_queue puts the 'largest' element (as per comparator) at the top.
        // So we want 'a < b' to return true when 'a' is better than 'b', making 'b' the top.
        // Time Complexity: O(N log K) where N is unique words, Space Complexity: O(K)
        auto comp = [](const pair<string,int> &a, const pair<string,int> &b) {
            if (a.second != b.second) return a.second > b.second;
            return a.first < b.first;
        };

        priority_queue<pair<string,int>, vector<pair<string,int>>, decltype(comp)> pq(comp);

        for(auto &p : it->second) {
            pq.push(p);
            if((int)pq.size() > k) {
                pq.pop(); // Replaces the worst retained candidate
            }
        }

        vector<pair<string,int>> res;
        while(!pq.empty()) {
            res.push_back(pq.top());
            pq.pop();
        }

        // Extracted from worst to best, so reverse it
        reverse(res.begin(), res.end());
        return res;
    }
};

class Query{
public:
    virtual void execute(VersionedIndex &idx)=0;
    virtual ~Query(){}
};

class WordQuery: public Query{
    string version;
    string word;

public:
    WordQuery(string v,string w):version(v),word(w){}

    void execute(VersionedIndex &idx) override {
        cout<<"Word count = "
            <<idx.wordCount(version,word)<<endl;
    }
};

class DiffQuery: public Query{
    string v1,v2,word;

public:
    DiffQuery(string a,string b,string w):v1(a),v2(b),word(w){}

    void execute(VersionedIndex &idx) override {
        cout<<"Difference = "
            <<idx.diff(v1,v2,word)<<endl;
    }
};

class TopQuery: public Query{
    string version;
    int k;

public:
    TopQuery(string v,int k):version(v),k(k){}

    void execute(VersionedIndex &idx) override {
        auto v=idx.topK(version, k);
        printTopK(v);
    }
};

string getRequiredArg(const vector<string>& args, const string& key) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == key) {
            if (i + 1 >= args.size() ||
                args[i + 1].empty() ||
                args[i + 1].rfind("--", 0) == 0) {
                throw runtime_error("Missing value for flag: " + key);
            }
            return args[i + 1];
        }
    }
    throw runtime_error("Missing required flag: " + key);
}

int parseInt(const string &s){
    if(s.empty()) throw runtime_error("Missing integer value");
    size_t pos;
    int res = stoi(s, &pos);
    if(pos != s.size()) throw runtime_error("Invalid integer format: " + s);
    return res;
}

int main(int argc,char* argv[]){
    try{
        auto start=high_resolution_clock::now();

        vector<string> args;
        for(int i=1; i<argc; i++)
            args.push_back(argv[i]);

        string query = getRequiredArg(args,"--query");
        int buffer = parseInt(getRequiredArg(args,"--buffer"));

        VersionedIndex index;
        unique_ptr<Query> q = nullptr;

        if(query=="word"){
            string file = getRequiredArg(args,"--file");
            string version = getRequiredArg(args,"--version");
            string word = getRequiredArg(args,"--word");

            index.buildIndex(version, file, buffer);
            q = make_unique<WordQuery>(version, word);
        }
        else if(query=="top"){
            string file = getRequiredArg(args,"--file");
            string version = getRequiredArg(args,"--version");
            int k = parseInt(getRequiredArg(args,"--top"));
            if(k <= 0) throw runtime_error("K must be positive");

            index.buildIndex(version, file, buffer);
            q = make_unique<TopQuery>(version, k);
        }
        else if(query=="diff"){
            string file1 = getRequiredArg(args,"--file1");
            string file2 = getRequiredArg(args,"--file2");
            string v1 = getRequiredArg(args,"--version1");
            string v2 = getRequiredArg(args,"--version2");
            if(v1 == v2) throw runtime_error("Version names must be different for diff");
            string word = getRequiredArg(args,"--word");

            index.buildIndex(v1, file1, buffer);
            index.buildIndex(v2, file2, buffer);
            q = make_unique<DiffQuery>(v1, v2, word);
        }
        else{
            throw runtime_error("Unknown query type: " + query);
        }

        q->execute(index);

        auto end=high_resolution_clock::now();
        double time = duration<double>(end-start).count();

        cout<<"Buffer Size: "<<buffer<<" KB"<<endl;
        cout<<"Execution Time: "<<time<<" seconds"<<endl;

    }catch(exception &e){
        cerr<<"Error: "<<e.what()<<endl;
        return 1;
    }
    return 0;
}