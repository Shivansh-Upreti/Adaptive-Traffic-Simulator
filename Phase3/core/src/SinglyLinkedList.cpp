#include "../include/SinglyLinkedList.hpp"

SinglyLinkedList::SinglyLinkedList(){
    head = nullptr;
    tail= nullptr;
    size = 0;
}

void SinglyLinkedList::pushBack(int value){
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

void SinglyLinkedList::traverse(){
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

void SinglyLinkedList::clear(){
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
