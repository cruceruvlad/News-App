#ifndef _FORUM_H
#define _FORUM_H

#include <cstddef>
#include <list>
#include <queue>
#include <algorithm>
#include "helpers.h"

class User{
    private:
        char ID[10];
        int socket;
        bool online;
        std::queue<mesaj_topic> inbox;
    public:
        User(char ID[],int socket){
            memcpy(this->ID,ID,10);
            this->socket = socket;
            online = true;
        }

        char* getID(){
            return ID;
        }

        int getSocket(){
            return socket;
        }

        bool isOnline(){
            return online;
        }

        void setSocket(int socket){
            this->socket = socket;
        }

        void setOnline(bool online){
            this->online = online;
        }

        void readInbox(){   // trimite mesajele din inbox la client
            mesaj_topic msj;
            while(!inbox.empty()){
                msj = inbox.front();
                inbox.pop();
                send(socket,&msj,sizeof(mesaj_topic),0);
            }
        }

        void add2Inbox(mesaj_topic msj){ 
            inbox.push(msj);
        }

};

class Topic{
    private:
        char name[50];
        std::vector<std::pair<User*,bool>> subscribers;  //subscriberii sunt de forma Pereche<utilizator,SF>
	//in readme am zis de SF=0/1 uitasem ca l-am facut bool

    public:
        Topic(char name[]){
          memcpy(this->name,name,50);
        }

        char* getName(){
            return name;
        }

        std::pair<User*,bool>* searchSubscriber(User *user){	// cauta un subscriber si returneaza perechea corespunzatoare. aici cred ca e cam 
            for(std::pair<User*,bool> &subscriber : subscribers) // useless sa intoarca perechea. mai bine ii dadea sa intoarca doar SF-ul coresp.
                if(subscriber.first == user)
                    return &subscriber;
            return nullptr;
        }

        void addSubscriber(User *user, bool SF){
            subscribers.push_back(std::make_pair(user,SF));
        }

        void deleteSubscriber(std::pair<User*,bool> &subs){
            subscribers.erase(remove(subscribers.begin(),subscribers.end(),subs),subscribers.end());
        }

        void sendToSubscribers(mesaj_topic msj){   // aici cu nebuneli cu daca e online senduieste, daca nu e dar SF=true pushBackuieste in inbox.
            for(std::pair<User*,bool> &subscriber : subscribers){  
                if(subscriber.first->isOnline() == true)
                    send(subscriber.first->getSocket(),&msj,sizeof(mesaj_topic),0);
                else if(subscriber.second==true)
                        subscriber.first->add2Inbox(msj);
            }
        }
};

class Forum{
    private:
        std::vector<User> users; 
        std::vector<Topic> topics;

        Topic* searchTopic(char name[]){	// cauta topic dupa nume
            for(Topic &topic : topics)
                if(strncmp(topic.getName(),name,50)==0)
                    return &topic;
            return nullptr;
        }

        User* searchUser(char ID[]){  // cauta user dupa id
            for(User &user : users)
                if(strncmp(user.getID(),ID,10)==0)
                    return &user;
            return nullptr;
        }

        User* searchUser(int socket){  // cauta user dupa socket-ul prin care comunica
            for(User &user : users)
                if(user.getSocket()==socket)
                    return &user;
            return nullptr;
        }

    public:
        int updateUsers(char ID[],int socket){  // asta face update la useri
            User *p = searchUser(ID);
            if(p!=nullptr){		// daca exista actualizeaza socket, online si citeste inboxul
                    p->setSocket(socket);
                    p->setOnline(true);
                    p->readInbox();
                    return 1;		// si returneaza ca e utilizator vechi
                }
            users.push_back(User(ID,socket));  // altfel adauga utilizator nou
            return 0;	// si returneaza ca e utilizator nou (Duh!)
        }

        char* disconectUser(int socket){  
            User *p = searchUser(socket);
            p->setOnline(false);
            p->setSocket(-1);
            return p->getID();
        }

        void subscribe(int socket, char topic[], bool SF){
           Topic *pT = searchTopic(topic);   // cauta topicul pe care se face subscribe
           User  *pU = searchUser(socket);   // cauta userul care a facut subscribe
           if(pT==nullptr){	//daca nu exista topic se creeaza unul si se adauga utilizatorul
                Topic newTopic(topic);
                newTopic.addSubscriber(pU,SF);
                topics.push_back(newTopic);
                }
           else{		// altfel cauta utilizatorul lista de subscriberi
                std::pair<User*,bool> *pS = pT->searchSubscriber(pU);
                if(pS==nullptr)
                    pT->addSubscriber(pU,SF);  // daca nu e il adauga
                else
                    pS->second = SF;  // daca e actualizeaza SF-ul
           }
        }
        void unsubscribe(int socket, char topic[]){  // fratele lui subscribe
            Topic *pT = searchTopic(topic);
            User *pU = searchUser(socket);
            if(pT!=nullptr){
                std::pair<User*,bool> *pS = pT->searchSubscriber(pU);
                pT->deleteSubscriber(*pS);
            }
        }

	//cauta topicul in lista de topicuri a forumului
	// si daca exista trimite mesajul la subscriberi(sau mrg il baga in inbox,dupa caz)
        void sendToSubscribers(mesaj_topic msj){  
            Topic *pT = searchTopic(msj.msj.topic);
            if(pT!=nullptr)
                pT->sendToSubscribers(msj);
        }
};

#endif
