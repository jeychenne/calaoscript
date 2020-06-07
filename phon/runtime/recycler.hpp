/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 22/05/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: backup garbage collector which detects reference cycles. The algorithm implements the synchronous         *
 * recycler described in _The Garbage Collection Handbook. The Art of Automatic Memory Management_, by R. Jones, A.   *
 * Hosking & E. Moss. (See p. 66 ff.). This algorithm was first proposed by Bacon & Rajan (2001).                     *
 * This implementation distinguishes between collectable and non-collectable objects. Non-collectable objects are     *
 * never candidates for garbage collection since they can't contain reference cycles (e.g. String). Collectable       *
 * objects become candidates as soon as there are more that one references pointing to them, since they might contain *
 * a cycle; however, if the object's reference count reaches 0, the object is detached from the GC chain and it is    *
 * destroyed.                                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONOMETRICA_RECYCLER_HPP
#define PHONOMETRICA_RECYCLER_HPP

namespace phonometrica {

class Collectable;

class Recycler final
{
public:

	void add_candidate(Collectable *obj);

	void remove_candidate(Collectable *obj);

private:

	// Doubly-linked list of candidates for garbage collection.
	Collectable *root = nullptr;
};

} // namespace phonometrica

#endif // PHONOMETRICA_RECYCLER_HPP
