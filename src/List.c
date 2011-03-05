/*
 * Copyright (c) 2010-2011 Kevin M. Bowling, <kevin.bowling@kev009.com>, USA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <craftd/List.h>

CDList*
CD_CreateList (void)
{
    CDList* self = CD_malloc(sizeof(CDList));

    if (!self) {
        return NULL;
    }

    self->list = kl_init(cdList);

    pthread_rwlock_init(&self->lock, NULL);
    pthread_mutex_init(&self->iterating, NULL);

    return self;
}

CDList*
CD_CloneList (CDList* self)
{
    CDList* cloned = CD_CreateList();

    CD_LIST_FOREACH(self, it) {
        CD_ListPush(cloned, CD_ListIteratorValue(it));
    }

    return cloned;
}

CDPointer*
CD_DestroyList (CDList* self)
{
    CDPointer* result = CD_ListClear(self);

    kl_destroy(cdList, self->list);

    pthread_mutex_destroy(&self->iterating);
    pthread_rwlock_destroy(&self->lock);

    CD_free(self);

    return result;
}

CDListIterator
CD_ListBegin (CDList* self)
{
    CDListIterator it;

    pthread_rwlock_rdlock(&self->lock);

    it.raw    = kl_begin(self->list);
    it.parent = self;

    pthread_rwlock_unlock(&self->lock);

    return it;
}

CDListIterator
CD_ListEnd (CDList* self)
{
    CDListIterator it;

    pthread_rwlock_rdlock(&self->lock);

    it.raw    = kl_end(self->list);
    it.parent = self;

    pthread_rwlock_unlock(&self->lock);

    return it;
}

CDListIterator
CD_ListNext (CDListIterator it)
{
    it.raw = kl_next(it.raw);

    return it;
}

size_t
CD_ListLength (CDList* self)
{
    size_t         result = 0;
    CDListIterator it;

    for (it = CD_ListBegin(self); !CD_ListIteratorIsEqual(it, CD_ListEnd(self)); it = CD_ListNext(it)) {
        result++;
    }

    return result;
}

bool
CD_ListIteratorIsEqual (CDListIterator a, CDListIterator b)
{
    return a.raw == b.raw && a.parent == b.parent;
}


CDPointer
CD_ListIteratorValue (CDListIterator it)
{
    return kl_val(it.raw);
}

CDList*
CD_ListPush (CDList* self, CDPointer data)
{
    pthread_rwlock_wrlock(&self->lock);
    pthread_mutex_lock(&self->iterating);

    *kl_pushp(cdList, self->list) = data;

    pthread_mutex_unlock(&self->iterating);
    pthread_rwlock_unlock(&self->lock);

    return self;
}

CDPointer
CD_ListShift (CDList* self)
{
    CDPointer result = CDNull;

    pthread_rwlock_wrlock(&self->lock);
    pthread_mutex_lock(&self->iterating);

    kl_shift(cdList, self->list, &result);

    pthread_mutex_unlock(&self->iterating);
    pthread_rwlock_unlock(&self->lock);

    return result;
}

CDPointer
CD_ListFirst (CDList* self)
{
    CDPointer result = CDNull;

    pthread_rwlock_rdlock(&self->lock);
    result = kl_val(kl_begin(self->list));
    pthread_rwlock_unlock(&self->lock);

    return result;
}

CDPointer
CD_ListLast (CDList* self)
{
    CDPointer result = CDNull;

    pthread_rwlock_rdlock(&self->lock);
    result = kl_val(kl_begin(self->list));
    pthread_rwlock_unlock(&self->lock);

    return result;
}

// FIXME: This looks like it can go wrong, maybe set wrlock and use backend stuff
CDPointer
CD_ListDelete (CDList* self, CDPointer data)
{
    CDPointer      result = CDNull;
    CDListIterator del;

    CD_LIST_FOREACH(self, it) {
        if (CD_ListIteratorValue(del = CD_ListNext(it)) == data) {
            result       = CD_ListIteratorValue(del);
            it.raw->next = CD_ListNext(del).raw;
            kmp_free(cdList, self->list->mp, del.raw);

            break;
        }
    }

    return result;
}

// FIXME: same as above
CDPointer
CD_ListDeleteAll (CDList* self, CDPointer data)
{
    CDPointer      result = CDNull;
    CDListIterator del;

    CD_LIST_FOREACH(self, it) {
        if (CD_ListIteratorValue(del = CD_ListNext(it)) == data) {
            result       = CD_ListIteratorValue(del);
            it.raw->next = del.raw->next;
            kmp_free(cdList, self->list->mp, del.raw);
        }
    }

    return result;
}

// FIXME: same as above
CDPointer*
CD_ListClear (CDList* self)
{
    CDPointer* result = (CDPointer*) CD_malloc(sizeof(CDPointer));
    size_t     i      = 0;

    while (CD_ListLength(self) > 0) {
        i++;

        result        = (CDPointer*) CD_realloc(result, sizeof(CDPointer) * (i + 1));
        result[i - 1] = CD_ListDeleteAll(self, CD_ListFirst(self));
    }

    result[i] = (CDPointer) NULL;

    return result;
}

bool
CD_ListStartIterating (CDList* self)
{
    pthread_mutex_lock(&self->iterating);

    return true;
}

bool
CD_ListStopIterating (CDList* self, bool stop)
{
    if (!stop) {
        pthread_mutex_unlock(&self->iterating);
    }

    return stop;
}
