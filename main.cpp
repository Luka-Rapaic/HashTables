#include <iostream>
#include <string>
#include <fstream>
#include <limits>
#include <random>

using namespace std;

class AdressFunction {
public:
    virtual int getAdress(int k, int a, int i) = 0;
};

class LinearHash : public AdressFunction {
public:
    LinearHash(int s): s(s) {};
    int getAdress(int k, int a, int i) override {
        return a + i * s;
    }
private:
    int s;
};

class HashTable {
public:
    HashTable(int size, int s): size(size), aSuccess(0), aUnsuccess(0), sSuccess(0), sUnsuccess(0), adress(new LinearHash(s)), table(new Key[size]) {};
    ~HashTable() {brisi();};
    virtual string* findKey(int k);
    virtual bool insertKey(int k, string s);
    virtual bool deleteKey(int k);
    int avgAccessSuccess();
    int avgAccessUnsuccess();
    int avgAccessUnsuccessEst();
    void resetStatistics();
    void clear();
    int keyCount();
    int tableSize();
    friend ostream& operator<< (ostream &os, const HashTable &ht);
    double fillRatio();
protected:
    struct Key {
        int key;
        string *data;
        Key(): key(0), data(nullptr) {};
    };
    int hash(int k);
    void brisi();
protected:
    int size;
    Key *table;
    AdressFunction *adress;
private:
    int aSuccess, aUnsuccess, sSuccess, sUnsuccess;
};

int HashTable::hash(int k) {
    return k % size;
}

void HashTable::brisi() {
    for (int i = 0; i < size; i++) {
        delete table[i].data;
    }
}

string* HashTable::findKey(int k) {
    int count = 1;
    int a = hash(k);

    if (table[a].key == k) {
        aSuccess++;
        sSuccess++;
        return table[a].data;
    }

    int i = 1, b = a;
    while (table[b].key && i < size) {
        count += 2;
        b = adress->getAdress(k, a, i) % size;
        if (table[b].key == k) {
            aSuccess += count;
            sSuccess++;
            return table[b].data;
        }
        i++;
    }
    count++;

    aUnsuccess += count;
    sUnsuccess++;
    return nullptr;
}

bool HashTable::insertKey(int k, string s) {
    int count = 2;
    int a = hash(k);
    int index = -1;

    if (table[a].key == 0) {
        aUnsuccess++;
        sUnsuccess++;
        table[a].key = k;
        table[a].data = new string(s);
        return true;
    } else if (table[a].key == -1) {
        index = a;
    } else if (table[a].key == k) {
        aSuccess += 2;
        sSuccess++;
        return false;
    }

    int i = 1, b = a;
    while (table[b].key && i < size) {
        count+=2;
        b = adress->getAdress(k, a, i) % size;
        if ((table[b].key == -1 || table[b].key == 0) && index == -1) {
            index = b;
        } else if (table[b].key == k) {
            aSuccess += count;
            sSuccess++;
            return false;
        }
        i++;
    }
    count++;

    if (index != -1) {
        aUnsuccess += count;
        sUnsuccess++;
        table[index].key = k;
        table[index].data = new string(s);
        return true;
    } else { //Ukoliko je tabela puna, ne povecava ni broj pristupa za uspesno ni za neuspesno pretrazivanje!
        cout << "Tabela je puna!" << endl;
    }

    return false;
}

bool HashTable::deleteKey(int k) {
    int a = hash(k);

    if (table[a].key == k) {
        table[a].key = -1;
        delete table[a].data;
        table[a].data = nullptr;
        return true;
    }

    int i = 1, b = a;
    while (table[b].key && i < size) {
        b = adress->getAdress(k, a, i) % size;
        if (table[b].key == k) {
            table[b].key = -1;
            delete table[b].data;
            table[b].data = nullptr;
            return true;
        }
        i++;
    }

    return false;
}

int HashTable::avgAccessSuccess() {
    return sSuccess ? aSuccess/sSuccess : 0;
}

int HashTable::avgAccessUnsuccess() {
    return sUnsuccess ? aUnsuccess/sUnsuccess : 0;
}

int HashTable::avgAccessUnsuccessEst() {
    int cnt = 0;
    for (int a = 0; a < size; a++) {
        int i = 0, b = a;
        while (table[b].key != 0 && table[b].key != -1) {
            cnt++;
            b = adress->getAdress(0, a, i);
        }
        cnt++;
    }
    return cnt/size;
}

void HashTable::resetStatistics() {
    aSuccess = 0;
    aUnsuccess = 0;
    sSuccess = 0;
    sUnsuccess = 0;
}

void HashTable::clear() {
    for (int i = 0; i < size; i++) {
        table[i].key = 0;
        delete table[i].data;
        table[i].data = nullptr;
    }
}

int HashTable::keyCount() {
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (table[i].key != 0 && table[i].key != -1) count++;
    }
    return count;
}

int HashTable::tableSize() {
    return size;
}

ostream& operator<< (ostream &os, const HashTable &ht) {
    string *str;
    for (int i = 0; i < ht.size; i++) {
        str = ht.table[i].data;
        str ? os << *str << endl : os << "EMPTY" << endl;
    }
    return os;
}

double HashTable::fillRatio() {
    return (double)keyCount()/tableSize();
}

class AdaptiveHashTable : public HashTable {
public:
    AdaptiveHashTable(int effThreshold, double frThreshold, int step, int size, int s);
    string* findKey(int k) override;
    bool insertKey(int k, string s) override;
    bool deleteKey(int k) override;
protected:
    void resize();
private:
    int effThreshold;
    double frThreshold;
    int step;
};

void AdaptiveHashTable::resize() {
    int oldSize = size;
    size += step;
    Key *newTable = new Key[size];

    for (int i = 0; i < oldSize; i++) {
        if (table[i].key == 0 || table[i].key == -1) continue;

        int k = table[i].key;
        string *str = table[i].data;
        int a = hash(k);

        if (newTable[a].key == 0) {
            newTable[a].key = k;
            newTable[a].data = str;
            continue;
        }

        int j = 1, b = a;
        while (newTable[b].key && j < size) {
            b = adress->getAdress(k, a, j) % size;
            j++;
        }

        newTable[b].key = k;
        newTable[b].data = str;
    }

    delete[] table;
    table = newTable;
    resetStatistics();
}

AdaptiveHashTable::AdaptiveHashTable(int effThreshold, double frThreshold, int step, int size, int s): HashTable(size, s) {
    if (effThreshold <= 0) {
        cout << "ERROR: effThreshold not valid!" << endl;
        exit(0);
    }
    this->effThreshold = effThreshold;

    if (frThreshold <= 0 || frThreshold > 1) {
        cout << "ERROR: frThreshold not valid!" << endl;
        exit(0);
    }
    this->frThreshold = frThreshold;

    if (step <= 0) {
        cout << "ERROR: step not valid!" << endl;
        exit(0);
    }
    this->step = step;
};

string* AdaptiveHashTable::findKey(int k) {
    string *str = HashTable::findKey(k);
    if (avgAccessSuccess() > effThreshold || avgAccessUnsuccess() > effThreshold) {
        resize();
    }
    return str;
}

bool AdaptiveHashTable::insertKey(int k, std::string s) {
    bool result = HashTable::insertKey(k, s);
    if (avgAccessSuccess() > effThreshold || avgAccessUnsuccess() > effThreshold || fillRatio() > frThreshold) {
        resize();
    }
    return result;
}

bool AdaptiveHashTable::deleteKey(int k) {
    bool result = HashTable::deleteKey(k);
    if (avgAccessSuccess() > effThreshold || avgAccessUnsuccess() > effThreshold) {
        resize();
    }
    return result;
}

struct KeyVal {
    int key;
    string val;
};

int getMin(fstream& fs) {
    int min = INT32_MAX;
    if (fs.is_open()) {
        fs.clear();
        fs.seekg(0);
        while (true) {
            int key; string str;
            fs >> str;
            if (fs.eof()) break;
            fs >> key;
            if (key < min) min = key;
        }
        return min;
    } else {
        cout << "ERROR: File not open!" << endl;
        exit(0);
    }
}

int getMax(fstream& fs) {
    int max = INT32_MIN;
    if (fs.is_open()) {
        fs.clear();
        fs.seekg(0);
        while (true) {
            int key; string str;
            fs >> str;
            if (fs.eof()) break;
            fs >> key;
            if (key > max) max = key;
        }
        return max;
    } else {
        cout << "ERROR: File not open!" << endl;
        exit(0);
    }
}

int getNumOfLines(fstream& fs) {
    int count = 0;
    if (fs.is_open()) {
        fs.clear();
        fs.seekg(0);
        while (!fs.eof()) {
            fs.ignore(numeric_limits<streamsize>::max(),'\n');
            count++;
        }
        return count;
    } else {
        cout << "ERROR: File not open!" << endl;
        exit(0);
    }
}

string atVal(fstream& fs, int val) {
    if (fs.is_open()) {
        fs.clear();
        fs.seekg(0);
        while (!fs.eof()) {
            string str;
            int key;
            fs >> str;
            fs >> key;
            if (key == val) {
                return str;
            }
        }
        return "";
    } else {
        cout << "ERROR: File not open!" << endl;
        exit(0);
    }
}

KeyVal atLine(fstream& fs, int index) {
    KeyVal keyVal;
    if (fs.is_open()) {
        fs.clear();
        fs.seekg(0);
        for (int i = 0; i < index; i++) {
            fs.ignore(numeric_limits<streamsize>::max(),'\n');
        }
        fs >> keyVal.val;
        fs >> keyVal.key;
        return keyVal;
    } else {
        cout << "ERROR: File not open!" << endl;
        exit(0);
    }
}

int main() {
    HashTable *ht = nullptr;
    fstream fs;
    int fMin;
    int fMax;
    int fRow;
    while (true) {
        cout << "~~~~Menu~~~~" << endl;
        cout << "~Hash table~" << endl;
        cout << "1. Create hash table" << endl;
        cout << "2. Create adaptive hash table" << endl;
        cout << "3. Print hash table" << endl;
        cout << "4. Clear hash table" << endl;
        cout << "5. Delete hash table" << endl;
        cout << "~Document~" << endl;
        cout << "6. Load document" << endl;
        cout << "7. Insert by key" << endl;
        cout << "8. Insert by row" << endl;
        cout << "9. Test performance" << endl;
        cout << "~Console~" << endl;
        cout << "10. Insert" << endl;
        cout << "11. Search" << endl;
        cout << "12. Delete" << endl;
        cout << "~Statitics~" << endl;
        cout << "14. Get statistics" << endl;
        cout << "15. Get key count" << endl;
        cout << "16. Get size" << endl;
        cout << "17. Reset statistics" << endl;
        cout << "~General~" << endl;
        cout << "18. Exit" << endl << endl;

        cout << "Please choose an option" << endl;
        int option;
        cin >> option;
        cout << endl;
        if (option < 1 || option > 18 || option == 13) {
            cout << "ERROR: Option not valid!" << endl << endl;
            continue;
        }

        switch(option) {
            case 1: {
                delete ht;
                ht = nullptr;
                cout << "Enter hash table size" << endl;
                int size;
                cin >> size;
                cout << "Enter adress function step" << endl;
                int step;
                cin >> step;
                if (size > 0 && step > 0) {
//                    cout << "Hash table created successfully!" << endl;
                    ht = new HashTable(size, step);
                } else {
                    cout << "ERROR: Failed to create hash table!" << endl;
                }
                cout << endl;
                break;
            }
            case 2: {
                delete ht;
                ht = nullptr;
                cout << "Enter adaptive hash table size" << endl;
                int size;
                cin >> size;
                cout << "Enter adress function step" << endl;
                int step;
                cin >> step;
                cout << "Enter adaptive hash table efficiency threshold" << endl;
                int effThreshold;
                cin >> effThreshold;
                cout << "Enter adaptive hash table fill ratio threshold" << endl;
                double frThreshold;
                cin >> frThreshold;
                cout << "Enter adaptive hash table size step" << endl;
                int sizeStep;
                cin >> sizeStep;
                if (size > 0 && step > 0) {
//                    cout << "Adaptive hash table created successfully!" << endl;
                    ht = new AdaptiveHashTable(effThreshold, frThreshold, sizeStep, size, step);
                } else {
                    cout << "ERROR: Failed to create adaptive hash table!" << endl;
                }
                cout << endl;
                break;
            }
            case 3: {
                if (ht) {
                    cout << *ht;
                } else {
                    cout << "ERROR: No hash table to display!" << endl;
                }
                cout << endl;
                break;
            }
            case 4: {
                if (ht) {
                    (*ht).clear();
//                    cout << "Hash table cleared successfully!" << endl;
                } else {
                    cout << "ERROR: No hash table to clear!" << endl;
                }
                cout << endl;
                break;
            }
            case 5: {
                if (ht) {
                    delete ht;
                    ht = nullptr;
//                    cout << "Hash table deleted successfully!" << endl;
                } else {
                    cout << "ERROR: No hash table to delete!" << endl;
                }
                cout << endl;
                break;
            }
            case 6: {
                if (fs.is_open()) fs.close();
                cout << "Enter file name" << endl;
                string fileName;
                cin >> fileName;
                fs.open(fileName);
                if (fs.is_open()) {
                    fMin = getMin(fs);
                    fMax = getMax(fs);
                    fRow = getNumOfLines(fs);
//                    cout << "File loaded successfully!" << endl;
                } else {
                    cout << "ERROR: Failed to load file!" << endl;
                }
                cout << endl;
                break;
            }
            case 7: { //TODO: Dodaj poruku ako fejluje (optional)
                if (!ht) {
                    cout << "ERROR: No hash table to insert into!" << endl;
                } else if (!fs.is_open()) {
                    cout << "ERROR: No file loaded!" << endl;
                } else {
                    cout << "Enter key" << endl;
                    int key;
                    cin >> key;
                    string str = atVal(fs, key);
                    if (str == "") {
                        cout << "ERROR: Key not found!" << endl;
                    } else {
                        (*ht).insertKey(key, str);
                    }
                }
                cout << endl;
                break;
            }
            case 8: {
                if (!ht) {
                    cout << "ERROR: No hash table to insert into!" << endl;
                } else if (!fs.is_open()) {
                    cout << "ERROR: No file loaded!" << endl;
                } else {
                    cout << "Enter row" << endl;
                    int row;
                    cin >> row;
                    if (row < 0 || row > fRow) {
                        cout << "ERROR: Row out of range!" << endl;
                    } else {
                        KeyVal keyVal = atLine(fs, row);
                        (*ht).insertKey(keyVal.key, keyVal.val);
                    }
                }
                cout << endl;
                break;
            }
            case 9: {
                if (!ht) {
                    cout << "ERROR: No hash table to test!" << endl;
                } else if (!fs.is_open()) {
                    cout << "ERROR: No file loaded!" << endl;
                } else {
                    cout << "Enter number of keys" << endl;
                    int n;
                    cin >> n;
                    for (int i = 0; i < n; i++) {
                        int row = rand() % fRow;
                        KeyVal kv = atLine(fs, row);
                        (*ht).insertKey(kv.key, kv.val);
                    }
                    for (int i = 0; i < n*10; i++) {
                        int key = rand() % (fMax + 1 - fMin) + fMin;
                        (*ht).findKey(key);
                    }
                    cout << "Results:" << endl;
                    cout << "Successful search, elements accessed on average: " << (*ht).avgAccessSuccess() << endl;
                    cout << "Unsuccessful search, elements accessed on average: " << (*ht).avgAccessUnsuccess() << endl;
                    cout << "Fill ratio: " << (*ht).fillRatio() << endl;
                }
                cout << endl;
                break;
            }
            case 10: {
                if (!ht) {
                    cout << "ERROR: No hash table to insert into!" << endl;
                } else {
                    cout << "Enter key" << endl;
                    int key;
                    cin >> key;
                    if (key <= 0) {
                        cout << "ERROR: Key not valid!" << endl;
                    } else {
                        cout << "Enter string" << endl;
                        string str;
                        cin >> str;
                        (*ht).insertKey(key, str);
                    }
                }
                cout << endl;
                break;
            }
            case 11: {
                if (!ht) {
                    cout << "ERROR: No hash table to search!" << endl;
                } else {
                    cout << "Enter key" << endl;
                    int key;
                    cin >> key;
                    if (key <= 0) {
                        cout << "ERROR: Key not valid!" << endl;
                    } else {
                        string *str = (*ht).findKey(key);
                        if (str) {
                            cout << "String found:" << endl;
                            cout << *str << endl;
                        } else {
                            cout << "String not found!" << endl;
                        }
                    }
                }
                cout << endl;
                break;
            }
            case 12: {
                if (!ht) {
                    cout << "ERROR: No hash table to delete from!" << endl;
                } else {
                    cout << "Enter key" << endl;
                    int key;
                    cin >> key;
                    if (key <= 0) {
                        cout << "ERROR: Key not valid!" << endl;
                    } else if ((*ht).deleteKey(key)) {
                        cout << "Key deleted successfully!" << endl;
                    } else {
                        cout << "ERROR: Key does not exist!" << endl;
                    }
                }
                cout << endl;
                break;
            }
            case 14: {
                if (!ht) {
                    cout << "ERROR: No hash table to get statistics from!" << endl;
                } else {
                    cout << "Statistics:" << endl;
                    cout << "Successful search, elements accessed on average: " << (*ht).avgAccessSuccess() << endl;
                    cout << "Unsuccessful search, elements accessed on average: " << (*ht).avgAccessUnsuccess() << endl;
                    cout << "Fill ratio: " << (*ht).fillRatio() << endl;
                }
                cout << endl;
                break;
            }
            case 15: {
                if (!ht) {
                    cout << "ERROR: No hash table to get key count from!" << endl;
                } else {
                    cout << "Key count: " << (*ht).keyCount() << endl;
                }
                cout << endl;
                break;
            }
            case 16: {
                if (!ht) {
                    cout << "ERROR: No hash table to get size from!" << endl;
                } else {
                    cout << "Hash table size: " << (*ht).tableSize() << endl;
                }
                cout << endl;
                break;
            }
            case 17: {
                if (!ht) {
                    cout << "ERROR: No hash table to reset statistics!" << endl;
                } else {
                    (*ht).resetStatistics();
                    cout << "Statistics reset successfully!" << endl;
                }
                cout << endl;
                break;
            }
            case 18: {
                delete ht;
                if (fs.is_open()) fs.close();
                cout << "Goodbye, hope to see you again :'(" << endl;
                return 0;
            }
        }
    }
}
