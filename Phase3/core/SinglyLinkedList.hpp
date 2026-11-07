#pragma once
#include <iostream>
using namespace std;

// singly linked list of ints, used to store the route of a vehicle.
// every node is created with new, so the list must delete every node again
class SinglyLinkedList{
    public:
        struct Node{
            int data;
            Node* next;
        };
        SinglyLinkedList(){
            head = nullptr;
            tail= nullptr;
            size = 0;
        }
        ~SinglyLinkedList(){
            clear();
        }

        // phase 3: a copy would share the same nodes and delete them twice,so no copying
        SinglyLinkedList(const SinglyLinkedList&) = delete;
        SinglyLinkedList& operator=(const SinglyLinkedList&) = delete;

        void pushBack(int value){
            Node* node = new Node;
            node->data = value;
            node->next = nullptr;

            if (tail == nullptr){
                head = node;
                tail = node;
            }
            else{
                tail->next = node;
                tail = node;
            }
            size++;
        }

        bool isEmpty(){
            return head == nullptr;
        }

        int Size(){
            return size;
        }

        Node* getHead(){
            return head;
        }

        void traverse(){
            if(isEmpty()){
                cout<<"Route is Empty!!"<<endl;
                return;
            }

            Node* temp = head;
            while (temp != nullptr){
                if(temp->next==nullptr){
                    cout<<temp->data;
                    break;
                }
                cout<<temp->data<<" -> ";
                temp = temp->next;

            }
        }

        //deletes every node. the next pointer is saved BEFORE delete, because 
        //after delete the node is gone and its next can no longer be read
        void clear(){
            Node* current = head;
            while (current != nullptr){
                Node* next = current->next;
                delete current;
                current = next;
            }
            head = nullptr;
            tail = nullptr;
            size = 0;
        }
    private:
        Node* head;
        Node* tail;
        int size;
};
