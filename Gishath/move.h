#pragma once
class Move {
public:
    Move(unsigned int from, unsigned int to, unsigned int flags);
    unsigned int getFrom();
    unsigned int getTo();
    unsigned int getFlags();
    void setTo(unsigned int to);
    void setFrom(unsigned int from);
    
    void operator=(Move a);
    bool operator==(Move a) const;
    bool operator!=(Move a) const;

protected:
    unsigned int m_Move;
};