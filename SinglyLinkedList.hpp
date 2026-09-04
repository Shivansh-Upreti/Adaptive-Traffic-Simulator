#pragma once

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