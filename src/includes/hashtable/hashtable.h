// namespace HashTable {

template <class ListHead, class KeyType, class DataType>
class HashTable {
 public:
  HashTable(int num_buckets = 1000);

  int length;

  ListHead* get_bucket(const KeyType k);
  ListHead* get_bucket_index(const int i);

  ~HashTable();

 private:
  ListHead *buckets;

  int hash(const KeyType k);
};

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
class HashTableWorker {
 public:
  bool insert(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, const DataType v); 
  bool search(HashTable<ListHead, KeyType, DataType> *table, const KeyType k);
  bool remove(HashTable<ListHead, KeyType, DataType> *table, const KeyType k);
  bool get(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, DataType& v);
  std::string visual(HashTable<ListHead, KeyType, DataType> *table);
  std::string histogram(HashTable<ListHead, KeyType, DataType> *table);

 private:
  ListHeadWorker worker;
};




template <class ListHead, class KeyType, class DataType>
HashTable<ListHead, KeyType, DataType>::HashTable(int num_buckets) {
  length = num_buckets;
  buckets = new ListHead[length];
}

template <class ListHead, class KeyType, class DataType>
ListHead* HashTable<ListHead, KeyType, DataType>::get_bucket(const KeyType k) {
  return &buckets[hash(k)];
}

template <class ListHead, class KeyType, class DataType>
ListHead* HashTable<ListHead, KeyType, DataType>::get_bucket_index(const int i) {
  if (0 <= i && i < length) {
    return &buckets[i];
  } else {
    return NULL;
  }
}

template <class ListHead, class KeyType, class DataType>
int HashTable<ListHead, KeyType, DataType>::hash(const KeyType k) {
  // return k % length;
  return std::hash<KeyType>{}(k) % length;
}

template <class ListHead, class KeyType, class DataType>
HashTable<ListHead, KeyType, DataType>::~HashTable() {
  delete [] buckets;
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
bool HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::insert(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, const DataType v) {
  ListHead *head = table->get_bucket(k);
  return worker.insert(head, k, v);
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
bool HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::search(HashTable<ListHead, KeyType, DataType> *table, const KeyType k) {
  ListHead *head = table->get_bucket(k);
  return worker.search(head, k);
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
bool HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::remove(HashTable<ListHead, KeyType, DataType> *table, const KeyType k) {
  ListHead *head = table->get_bucket(k);
  return worker.remove(head, k);
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
bool HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::get(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, DataType& v) {
  ListHead *head = table->get_bucket(k);
  return worker.get(head, k, v);
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
std::string HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::visual(HashTable<ListHead, KeyType, DataType> *table) {
  std::stringstream ss;
  for (int i = 0; i < table->length; i++) {
    ss << "[" << i << "] => ";
    ss << worker.visual(table->get_bucket_index(i));
    ss << "\n";
  }
  return ss.str();
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
std::string HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::histogram(HashTable<ListHead, KeyType, DataType> *table) {
  std::stringstream ss;
  for (int i = 0; i < table->length; i++) {
    ss << "[" << i << "] => ";
    ss << worker.histogram(table->get_bucket_index(i));
    ss << "\n";
  }
  return ss.str();
}

// }