#ifndef _BOUNDED_FIFO_QUEUE_INCLUDED
#define _BOUNDED_FIFO_QUEUE_INCLUDED

template<typename T, int SIZE>
class CBoundedQueue
{
public:
	CBoundedQueue() : head(0), tail(0), count(0) {};

	int Enqueue(const T &nValue);
	int Dequeue(T* pValue);
	bool IsEmpty() { return head==tail && count==0; };
	bool IsFull() { return head==tail && count==SIZE; };

	void Clear();

private:
	T m_arBuf[SIZE];
	int head;
	int tail;
	int count;
};

template<typename T, int SIZE>
int CBoundedQueue<T,SIZE>::Enqueue(const T &nValue)
{
	if (IsFull()) return -2;
	m_arBuf[tail] = nValue;	
	tail = (tail+1) % SIZE;
	count++;

	return 0;
}

template<typename T, int SIZE>
int CBoundedQueue<T,SIZE>::Dequeue(T* pValue)
{
	if (IsEmpty()) return -1;
	*pValue = m_arBuf[head];
	head = (head+1) % (SIZE);
	count--;
	return 0;
}

/*template<typename K>
void CFlatMap<K>::Insert(K nKey, unsigned int nValue)
{
	InsertIndex(nKey, nValue);
}

template<typename K>
int CFlatMap<K>::Find(K nKey)
{
	int ind = FindIndex(nKey);
	if (ind >= 0)
		return m_arValue[ind];	
	return -1;
}

template<typename K>
void CFlatMap<K>::Clear()
{
	m_arKey.clear();
	m_arValue.clear();
}

template<typename K>
void CFlatMap<K>::Delete(K nKey)
{
	int nIndex = FindIndex(nKey);
	if (nIndex<0) return;

	m_arKey.erase(m_arKey.begin()+nIndex);
	m_arValue.erase(m_arValue.begin()+nIndex);
}

template<typename K>
unsigned int &CFlatMap<K>::operator[] (K nKey)
{
	int nIndex = FindIndex(nKey);
	if (nIndex>=0) return m_arValue[nIndex];

	nIndex = InsertIndex(nKey, 0);
	return m_arValue[nIndex];
}

template<typename K>
int CFlatMap<K>::InsertIndex(K nKey, unsigned int nValue)
{
	int nIndex = FindIndex(nKey);

	if (nIndex>=0)
	{
		// Update existng key/value pair
		m_arValue[nIndex] = nValue;
		return nIndex;
	}

	// Insert new key/value pair
	m_arKey.push_back(nKey);
	m_arValue.push_back(nValue);

	nIndex = int(m_arKey.size())-1;
	K tmpKey;
	unsigned int tmpValue;
	while (nIndex>0 && m_arKey[nIndex-1]>m_arKey[nIndex])
	{
		tmpKey = m_arKey[nIndex-1];
		m_arKey[nIndex-1] = m_arKey[nIndex];
		m_arKey[nIndex] = tmpKey;

		tmpValue = m_arValue[nIndex-1];
		m_arValue[nIndex-1] = m_arValue[nIndex];
		m_arValue[nIndex] = tmpValue;
		nIndex--;
	}
	return nIndex;
}

template<typename K>
int CFlatMap<K>::FindIndex(K nKey)
{
	K v;
	int mid;
	int l=0, r=int(m_arKey.size())-1;

	while (l<=r)
	{
		mid = l + (r - l)/2;
		v = m_arKey[mid];
		if (nKey < v)
			r = mid - 1;
		else if (nKey > v)
			l = mid + 1;
		else
			return mid;
	}

	return -1;
}*/

#endif //_BOUNDED_FIFO_QUEUE_INCLUDED
