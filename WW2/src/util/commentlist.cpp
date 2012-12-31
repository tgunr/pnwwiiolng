#include <syscfg.h> // Path is project specific.
#include <string>
#include <iostream>
#include <sstream>
#include <map>

#include <osg/Node>
#include <osg/Notify>
#include <osgDB/readFile>

using namespace std;

class CommentList : public osg::NodeVisitor
{
public:
	CommentList() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), findmode(false) {}

	virtual void apply(osg::Node& node);
	void print(map<string, set<string> >&);

	map<string, set<string> > keyClassComments;
	map<string, set<string> > keyCommentComments;
	set<string> findComments;
	string filename;
	bool findmode;
};

int main(int argc, char** argv)
{
	osg::setNotifyLevel(osg::FATAL);

	CommentList cl;

	osgDB::FilePathList& datapathlist = osgDB::Registry::instance()->getDataFilePathList();

	datapathlist.push_back("to");
	datapathlist.push_back("bld");
	datapathlist.push_back("brdg");
	datapathlist.push_back("ag");
	datapathlist.push_back("cap");
	datapathlist.push_back("fac");

	int n = 1;
	for(; n < argc; n++)
	{
		string arg = argv[n];
		if(arg[0] != '-') break;
		if(arg == "-find")
		{
			cl.findmode = true;
			cl.findComments.insert(argv[n+1]);
			n++;
		}
	}

	for(int i = n; i < argc; i++)
	{
		cl.filename = argv[i];
		cl.filename += ".flt";
		osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(cl.filename);
		if(!node.valid()) continue;
		cl.apply(*node);
	}

	if(!cl.findmode)
	{
		cl.print(cl.keyClassComments);
		cout << "\n--\n\n";
		cl.print(cl.keyCommentComments);
	}
	return 0;
}

void CommentList::apply(osg::Node& node)
{
	unsigned int n = node.getNumDescriptions();

	for(unsigned int i = 0; i < n ;i++)
	{
		string& comment = node.getDescription(i);
		string arg1,arg2;
		stringstream ss(comment);
		ss >> arg1 >> arg2;

		if(findmode)
		{
			if(findComments.find(arg1) != findComments.end())
			{
				cout << arg1 << " " << filename << ": " << node.getName() << "\n";
			}
		}else
		{
			if(!arg1.empty())
			{
				keyClassComments[node.className()].insert(arg1);
				keyCommentComments[arg1].insert(node.className());
			}
		}
	}

	traverse(node);
}

void CommentList::print(map<string, set<string> >& comments)
{
	map<string, set<string> >::iterator it = comments.begin();

	string prevKey;

	for(; it != comments.end(); ++it)
	{
		string key = it->first;
		if(key != prevKey)
		{
			if(!prevKey.empty()) cout << "\n";
			cout << key << ": ";
			prevKey = key;
		}

		set<string>::iterator val = it->second.begin();
		for(; val != it->second.end(); ++val)
		{
			cout << *val << " ";
		}

	}
	cout << "\n";
}