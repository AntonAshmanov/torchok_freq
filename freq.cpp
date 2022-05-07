#include <iostream>
#include <fstream>
#include <cctype>
#include <ios>
#include <chrono>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <tuple>
#include <algorithm>
#include <functional>

static size_t n = 0;
struct Node {
    int* counters = nullptr;
    Node* kids = nullptr;
    
    void Init() {
        if (!counters)
            InitCounters();
        if (!kids)    
            kids = new Node[26];
    }

    void InitCounters() {
        counters = new int[26];
        for (size_t i = 0; i < 26; ++i) {
            counters[i] = 0;
        }
    }

    ~Node() {
        delete[] counters;
        delete[] kids;
    }
};

void TrieOrder(Node* root, std::string& str, std::vector<std::pair<std::string, size_t>>& v) {
    if (!root || !root->counters)
        return;

    for (std::size_t i = 0; i < 26; ++i) {
        str += char('a' + i);
        if (root->counters[i]) {
            //std::cout << str << " " << root->counters[i] << std::endl;
            v.push_back({str, root->counters[i]});
        }
        str.pop_back();    
    }    

    if (root->kids)
        for (std::size_t i = 0; i < 26; ++i) {
            str += char('a' + i);
            TrieOrder(&root->kids[i], str, v);
            str.pop_back();
        }
}

int main() {

    Node* root = new Node;

    char prev_ch = '\0';
    char ch = '\0';

    Node* cur_node = root;
    size_t prev_index = 0;
    std::string str;
    
    int fd = open("pg.txt", O_RDONLY, (mode_t)0600);
    struct stat statbuf;
    int err = fstat(fd, &statbuf);
    unsigned char* map = reinterpret_cast<unsigned char*>(mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0));

    if (map == MAP_FAILED){
        close(fd);
        exit(EXIT_FAILURE);
    }

    std::chrono::steady_clock::time_point begin2 = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < statbuf.st_size; ++i) { 
        // середина слова
        ch = map[i];
        if (std::isalpha(ch) &&  std::isalpha(prev_ch)) {
            ch = std::tolower(ch);
            str += ch;
            prev_index = prev_ch - 'a';
            if (!cur_node->kids) {
                cur_node->Init();
            } 
            cur_node = &cur_node->kids[prev_index]; 
            prev_ch = ch; 
        } 
        // начало слова
        else if (std::isalpha(ch) &&  !std::isalpha(prev_ch)) {
            cur_node = root;
            prev_ch = std::tolower(ch);
            str += prev_ch;
        }
        // конец слова 
        else if (!std::isalpha(ch) && std::isalpha(prev_ch)) {
            if (!cur_node->counters)
                cur_node->InitCounters();
            prev_index = prev_ch - 'a';    
            ++cur_node->counters[prev_index];
            cur_node = root;
            prev_ch = '\0';
            str.clear();
        } else {
            prev_ch = std::tolower(ch);
            continue;
        }
    }

    std::chrono::steady_clock::time_point end2 = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end2 - begin2).count() << "[µs]" << std::endl;

    std::string word;
    
    std::chrono::steady_clock::time_point begin3 = std::chrono::steady_clock::now();
    std::vector<std::pair<std::string, size_t>> all_words;
    all_words.reserve(50000);
    TrieOrder(root, word, all_words);
    all_words.shrink_to_fit();
    size_t topK = 100;
    auto it_end = topK < all_words.size() ? all_words.begin() + topK : all_words.end();
    std::partial_sort(all_words.begin(), it_end, all_words.end(),
        [](const std::pair<std::string, size_t>& l, const std::pair<std::string, size_t>& r)
        {return std::tie(l.second, l.first) > std::tie(r.second, r.first);});
    size_t i = 1;    
    for (auto it = all_words.begin(); it != it_end; ++it) {
        std::cout << i++ << ": " << it->first << " " << it->second << std::endl;
    }
    std::chrono::steady_clock::time_point end3 = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end3 - begin3).count() << "[µs]" << std::endl;

    if (munmap(map, statbuf.st_size) == -1) {
        close(fd);
        perror("Error un-mmapping the file");
        exit(EXIT_FAILURE);
    }
    close(fd);
    delete root;
    return 0;
}