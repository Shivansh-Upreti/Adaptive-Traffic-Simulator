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

        SinglyLinkedList();

        // phase 3: the list now frees its own nodes when it is destroyed
        ~SinglyLinkedList(){
            clear();
        }

        // phase 3: a copy would share the same nodes and delete them twice, so no copying
        SinglyLinkedList(const SinglyLinkedList&) = delete;
        SinglyLinkedList& operator=(const SinglyLinkedList&) = delete;

        void pushBack(int value);

        bool isEmpty(){
            return head == nullptr;
        }

        int Size(){
            return size;
        }

        Node* getHead(){
            return head;
        }

        void traverse();

        // deletes every node. the next pointer is saved BEFORE delete, because
        // after delete the node is gone and its next can no longer be read
        void clear();
    private:
        Node* head;
        Node* tail;
        int size;
};
