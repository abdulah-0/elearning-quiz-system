#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <ctime>
using namespace std;

struct Question 
{
    string text;
    char answer;      
    double difficulty;  
};

struct QuestionNode 
{
    Question data;
    QuestionNode* next;
    QuestionNode(const Question& q):data(q),next(NULL) {}
};

struct StudentRecord 
{
    string name;
    int correctCount;
    int totalAsked;
    double theta;
    StudentRecord* next;
    StudentRecord(const string& n,int c,int t,double th):name(n),correctCount(c),totalAsked(t),theta(th),next(NULL) {}
};

void loadRecords(const string& filename,StudentRecord*& head) 
{
    ifstream in(filename.c_str());
    if(!in) 
		return;  
    string line;
    while(getline(in, line)) 
	{
        if(line.empty()) 
			continue;
        stringstream ss(line);
        string name,sc,sa,sth;
        if(!getline(ss,name,'|'))     
			continue;
        if(!getline(ss,sc,'|'))     
			continue;
        if(!getline(ss,sa,'|'))     
			continue;
        if(!getline(ss,sth))           
			continue;
        int correct=atoi(sc.c_str());
        int asked=atoi(sa.c_str());
        double th=atof(sth.c_str());
        StudentRecord* rec=new StudentRecord(name,correct,asked,th);
        rec->next=head;
        head=rec;
    }
    in.close();
}

void saveRecords(const string& filename,StudentRecord* head) 
{
    ofstream out(filename.c_str());
    if(!out) 
	{
        cout<<"Warning: could not write "<<filename<<"\n";
        return;
    }
	StudentRecord* stack[1024];
    int count=0;
    for(StudentRecord* p=head;p&&count<1024;p=p->next)
        stack[count++]=p;
    for(int i=count-1;i>=0;--i) 
	{
        StudentRecord* r=stack[i];
        out<<r->name<<"|"<< r->correctCount<<"|"<< r->totalAsked<<"|"<<r->theta<< "\n";
    }
    out.close();
}

void loadQuestions(const string& filename,QuestionNode*& head) 
{
    ifstream in(filename.c_str());
    if(!in) 
	{
        cout<<"Error: could not open"<<filename<<"\n";
        exit(1);
    }
    string line;
    QuestionNode* tail=NULL;
    while(getline(in,line)) 
	{
        if (line.empty() || (line.size()>1 && line[0]=='/' && line[1]=='/'))
            continue;
        size_t p1=line.find('|');
        size_t p2=(p1==string::npos ? string::npos : line.find('|', p1+1));
        if(p1==string::npos || p2==string::npos) 
		{
            cout<<"Error: bad format in questions file\n";
            exit(1);
        }
        Question q;
        q.text=line.substr(0,p1);
        q.answer=line[p1+1];
        if (q.answer!='y' && q.answer!='Y' && q.answer!='n' && q.answer!='N') 
		{
            cout<<"Error: answer must be 'y' or 'n'\n";
            exit(1);
        }
        if(q.answer=='Y') 
			q.answer='y';
        if(q.answer=='N') 
			q.answer='n';
        q.difficulty=atof(line.substr(p2+1).c_str());
        QuestionNode* node=new QuestionNode(q);
        if (!head) 
			head=node;
        else       
			tail->next=node;
        tail=node;
    }
    in.close();
}

double probability(double theta,double b) 
{
    return 1.0/(1.0+exp(-(theta-b)));
}

Question pickNextQuestion(double theta,QuestionNode*& head) 
{
    const double NOISE_SCALE=1e-3;
    double bestScore=1e100;           
    QuestionNode *bestPrev=NULL,*bestNode=NULL;
    QuestionNode *prev=NULL,*curr=head;
    while(curr) 
	{
        double dist=fabs(curr->data.difficulty-theta);
        double noise=(rand()/double(RAND_MAX))*NOISE_SCALE;
        double score=dist+noise;
        if(score<bestScore) 
		{
            bestScore=score;
            bestPrev=prev;
            bestNode=curr;
        }
        prev=curr;
        curr=curr->next;
    }
    if(!bestNode) 
	{
        cout<<"Error: no question to pick\n";
        exit(1);
    }
    if(bestPrev)
		bestPrev->next=bestNode->next;
    else    
		head=bestNode->next;
    Question q=bestNode->data;
    delete bestNode;
    return q;
}

int main() 
{
    srand((unsigned)time(NULL));
    StudentRecord* recordsHead=NULL;
    loadRecords("records.txt", recordsHead);
    cout << "Enter your name: ";
    string studentName;
    getline(cin, studentName);
    if (studentName.empty()) 
	{
        cout<<"Name cannot be empty. Exiting.\n";
        return 1;
    }
    QuestionNode* questionHead=NULL;
    loadQuestions("questions.txt",questionHead);
    const int QUIZ_SIZE=5;
    double theta=0.0;
    const double learningRate=1.0;
    int asked=0;
    int correctCount=0;
    cout<<"\n=== Adaptive Quiz for "<<studentName<<" ("<<QUIZ_SIZE<<" questions) ===\n\n";
    while(asked<QUIZ_SIZE && questionHead) 
	{
        Question q=pickNextQuestion(theta, questionHead);
        cout<<"Q"<<(asked+1)<<": "<<q.text<<" (y/n)? ";
        char resp;
        cin>>resp;
        if (!(resp=='y'||resp=='Y'||resp=='n'||resp=='N')) 
		{
            cout<<"Please enter y or n\n";
            continue;
        }
        if(resp=='Y') 
			resp='y';
        if(resp=='N') 
			resp='n';
        bool isCorrect=(resp==q.answer);
        if(isCorrect) 
		{
            cout<<"Correct!\n\n";
            ++correctCount;
        } 
		else 
		{
            cout<<"Incorrect. (Answer: "<<(q.answer=='y'?"yes":"no")<<")\n\n";
        }
        double p= probability(theta,q.difficulty);
        double u=isCorrect?1.0:0.0;
        theta +=learningRate*(u-p);
        ++asked;
    }
    double scorePct=(100.0*correctCount)/asked;
    cout<<"=== Quiz Finished ===\n"<<"Student: "<<studentName<<"\n"<<"Score: "<<correctCount<<" / "<<asked<<" ("<<scorePct<<"%)\n"<<"Final ability (?): "<<theta<<"\n";
    StudentRecord* rec = new StudentRecord(studentName, correctCount, asked, theta);
    rec->next=recordsHead;
    recordsHead=rec;
    saveRecords("records.txt",recordsHead);
    return 0;
}

