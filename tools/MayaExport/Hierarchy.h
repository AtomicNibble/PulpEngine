#pragma once


#ifndef _X_HIERARCHY_H_
#define _X_HIERARCHY_H_


template<typename T>
class Hierarchy
{
public:
	typedef T type;

	Hierarchy();
	~Hierarchy();


	void	setOwner(type *object);
	type*	owner(void) const;
	void	setParent(Hierarchy& node);
	bool	isParentedBy(const Hierarchy& node) const;	// checks all parents
	void	makeBrotherAfter(Hierarchy& node);			// make the node a brother afrer the passed in node.

	type*	parent(void) const;
	type*	child(void) const;
	type*	brother(void) const;	  // next node with same parent.
	type*	priorBrother(void) const; // previous node with same parent.
	type*	next(void) const;
	type*	nextLeaf(void) const;


	void	removeFromParent(void);
	void	removeFromHierarchy(void);

private:
	Hierarchy<type>* priorBrotherNode(void) const;	// previous node with the same parent

private:
	Hierarchy* parent_;
	Hierarchy* child_;
	Hierarchy* brother_;

	type*		owner_;

};


// get kinky.

template<typename T>
Hierarchy<T>::Hierarchy()
{
	parent_ = nullptr;
	child_ = nullptr;
	brother_ = nullptr;
	owner_ = nullptr;
}

template<typename T>
Hierarchy<T>::~Hierarchy()
{
	removeFromHierarchy(); // Get the FUCK out !
}


template<typename T>
void Hierarchy<T>::setOwner(type* obj)
{
	owner_ = obj;
}

template<typename T>
typename Hierarchy<T>::type* Hierarchy<T>::owner(void) const
{
	return owner_;
}

template<typename T>
void Hierarchy<T>::setParent(Hierarchy& node)
{
	removeFromParent();

	parent_ = &node;
	brother_ = node.child_;
	node.child_ = this;
}

template<typename T>
bool Hierarchy<T>::isParentedBy(const Hierarchy& node) const
{
	if (parent_ == &node) {
		return true;
	}

	if (parent_) {
		return parent_->isParentedBy(node);
	}

	return false;
}

template<typename T>
void Hierarchy<T>::makeBrotherAfter(Hierarchy& node)
{
	removeFromParent();

	parent_ = node.parent_;
	brother_ = node.brother_;
	node.brother_ = this;
}

template<typename T>
typename Hierarchy<T>::type* Hierarchy<T>::parent(void) const
{
	if (parent_) {
		return parent_->owner_;
	}
	return nullptr;
}

template<typename T>
typename Hierarchy<T>::type* Hierarchy<T>::child(void) const
{
	if (child_) {
		return child_->owner_;
	}
	return nullptr;
}

template<typename T>
typename Hierarchy<T>::type* Hierarchy<T>::brother(void) const
{
	if (brother_) {
		return brother_->owner_;
	}
	return nullptr;
}


template<typename T>
typename Hierarchy<T>::type* Hierarchy<T>::priorBrother(void) const
{
	auto* prior = priorBrotherNode();
	if (prior) {
		return prior->owner_;
	}

	return nullptr;
}

template<typename T>
typename Hierarchy<T>::type* Hierarchy<T>::next(void) const
{
	const Hierarchy<type>* node;

	if (child_) { // return the child.
		return child_->owner_;
	}

	node = this;
	// go back up untill we have a brother.
	while (node && node->brother_ == nullptr) {
		node = node->parent_;
	}

	// is there a next one ?
	if (node) {
		return node->brother_->owner_;
	}

	return nullptr;
}

template<typename T>
typename Hierarchy<T>::type* Hierarchy<T>::nextLeaf(void) const
{
	const Hierarchy<type> *node;

	if (child) {
		node = child_;
		while (node->child_) {
			node = node->child_;
		}

		return node->owner_;
	}
	
	node = this;
	while (node && node->brother_ == nullptr) {
		node = node->parent_;
	}

	if (node) {
		node = node->brother_;
		while (node->child_) {
			node = node->child_;
		}

		return node->owner_;
	}

	return nullptr;
}


template<typename T>
void Hierarchy<T>::removeFromParent(void) {
	Hierarchy<type> *prev;

	if (parent_) {
		prev = priorBrotherNode();
		if (prev) {
			prev->brother_ = brother_;
		}
		else {
			parent_->child_ = brother_;
		}
	}

	parent_ = nullptr;
	brother_ = nullptr;
}


template<typename T>
void Hierarchy<T>::removeFromHierarchy(void) {
	Hierarchy<type> *parentNode;
	Hierarchy<type> *node;

	parentNode = parent_;
	removeFromParent();

	if (parentNode) {
		while (child_) {
			node = child_;
			node->removeFromParent();
			node->setParent(*parentNode);
		}
	}
	else {
		while (child_) {
			child_->removeFromParent();
		}
	}
}

template<typename T>
Hierarchy<T>* Hierarchy<T>::priorBrotherNode(void) const
{
	if (!parent_ || (parent_->child_ == this)) {
		return nullptr;
	}

	Hierarchy<type>* prev;
	Hierarchy<type>* node;

	node = parent_->child_;
	prev = nullptr;
	while ((node != this) && (node != nullptr)) {
		prev = node;
		node = node->brother_;
	}

	return prev;
}

#endif // !_X_HIERARCHY_H_
