﻿#pragma

#include "IntDictionary.cpp"
#include <map>
#include <vector>
#include <string>
#include "TrieNode.h"
using std::map;
using std::vector;
using std::string;

class BaseSearchEx
{
protected:
	unsigned short* _dict;
	int* _first;

	IntDictionary* _nextIndex;
	int* _end;
	int* _resultIndex;
	int* _keywordLengths;

protected:
	virtual void SetKeywords2(string _keywords[])
	{
		TrieNode root;
		map<int, vector<TrieNode>> allNodeLayers;
		int kindex = 0;

		for (size_t i = 0; i < _keywords->size(); i++) /// _keywords->size() 这个值有问题
		{
			string p = _keywords[i];

			TrieNode nd = root;
			for (size_t j = 0; j < p.length(); j++)   //  这个 p.length() 对中文有问题
			{
				nd = nd.Add((char)p.c_str()[j]);   // 返回是byte 不是char
				if (nd.Layer == 0) {
					nd.Layer = j + 1;
					auto find = allNodeLayers.find(nd.Layer);
					if (find == allNodeLayers.end())
					{
						vector<TrieNode> trieNodes;
						trieNodes.push_back(nd);
						allNodeLayers[nd.Layer] = trieNodes;
					}
					else
					{
						find->second.push_back(nd);
					}
				}
			}
		}
		vector<TrieNode> allNode;
		allNode.push_back(root);
		for (size_t i = 0; i < allNodeLayers.size(); i++)
		{
			auto item = allNodeLayers.at(i);
			for (size_t j = 0; j < item.size(); j++)
			{
				allNode.push_back(item[j]);
			}
		}
		allNodeLayers.clear();

		for (size_t i = 0; i < allNode.size(); i++)
		{
			TrieNode nd = allNode[i];
			nd.Index = i;
			TrieNode r = *(nd.Parent->Failure);
			char c = nd.Char;
			while (&r != NULL && (r.m_values.size() == 0 || r.m_values.find(c) == r.m_values.end())) r = *r.Failure;
			if (&r == NULL)
			{
				nd.Failure = &(root);
			}
			else
			{
				nd.Failure = &(r.m_values[c]);
				if (nd.Failure->Results.size() > 0) {
					for (size_t i = 0; i < nd.Failure->Results.size(); i++)
					{
						nd.SetResults(nd.Failure->Results[i]);
					}
				}
			}
		}
		root.Failure = &(root);

		string stringBuilder = "";
		for (int i = 1; i < allNode.size(); i++) {
			stringBuilder += (allNode[i].Char);
		}
		int length = CreateDict(stringBuilder);
		stringBuilder.clear();

		int first[0x10000];
		if (allNode[0].m_values.size() > 0) {
			for (size_t i = 0; i < allNode[0].m_values.size(); i++)
			{
				auto it = allNode[0].m_values.at(i);
				char key = (char)_dict[it.Char];
				first[key] = it.Index;
			}
		}
		_first = first;

		vector<int> resultIndex2;
		vector<bool> isEndStart;
		int len = allNode.size();
		IntDictionary* nextIndex2 = new IntDictionary[len];

		for (int i = allNode.size() - 1; i >= 0; i--) {
			map<unsigned short, int> dict;
			vector<int> result;
			TrieNode oldNode = allNode[i];

			if (oldNode.m_values.size() > 0) {
				for (size_t i = 0; i < oldNode.m_values.size(); i++)
				{
					char key = (char)oldNode.m_values.at(i).Char;
					int index = oldNode.m_values.at(i).Index;
					dict[key] = index;
				}
			}
			if (oldNode.Results.size() > 0) {
				for (size_t i = 0; i < oldNode.Results.size(); i++)
				{
					result.push_back(oldNode.Results[i]);
				}
			}

			oldNode = *oldNode.Failure;
			while (oldNode.Index != root.Index) {
				if (oldNode.m_values.size() > 0) {
					for (size_t i = 0; i < oldNode.m_values.size(); i++)
					{
						char key = (char)oldNode.m_values.at(i).Char;
						int index = oldNode.m_values.at(i).Index;
						if (dict.find(key) == dict.end())
						{
							dict[key] = index;
						}
					}
				}
				if (oldNode.Results.size() > 0) {
					for (size_t i = 0; i < oldNode.Results.size(); i++)
					{
						int idx = oldNode.Results[i];
						bool find = false;
						for (size_t j = 0; j < result.size(); j++)
						{
							if (result[i] == idx)
							{
								find = true;
								break;
							}
						}
						if (find == false)
						{
							result.push_back(oldNode.Results[i]);
						}
					}
				}
				oldNode = *(oldNode.Failure);
			}
			nextIndex2[i] = *(new IntDictionary(dict));

			if (result.size() > 0) {
				for (int j = result.size() - 1; j >= 0; j--) {
					resultIndex2.push_back(result[j]);
					isEndStart.push_back(false);
				}
				isEndStart[isEndStart.size() - 1] = true;
			}
			else {
				resultIndex2.push_back(-1);
				isEndStart.push_back(true);
			}
		}

		allNode.clear();
		_nextIndex = nextIndex2;

		vector<int> resultIndex;
		vector<int> end;

		for (int i = isEndStart.size() - 1; i >= 0; i--) {
			if (isEndStart[i]) {
				end.push_back(resultIndex.size());
			}
			if (resultIndex2[i] > -1) {
				resultIndex.push_back(resultIndex2[i]);
			}
		}
		end.push_back(resultIndex.size());

		_resultIndex = new int[resultIndex.size()];
		for (size_t i = 0; i < resultIndex.size(); i++)
		{
			_resultIndex[i] = resultIndex[i];
		}
		_end = new int[end.size()];
		for (size_t i = 0; i < end.size(); i++)
		{
			_end[i] = end[i];
		}
	}


private:
	int CreateDict(string keywords) {
		_dict = new unsigned short[0x10000];
		map<char, unsigned int> dictionary;
		int index = 1;
		for (size_t i = 0; i < keywords.size(); i++)
		{
			char item = keywords.at(i);
			if (_dict[item] == 0)
			{
				_dict[item] = index;
				index++;
			}
		}
		return index - 1;
	}

};