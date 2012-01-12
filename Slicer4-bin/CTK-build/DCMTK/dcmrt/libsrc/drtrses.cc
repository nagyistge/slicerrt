/*
 *
 *  Copyright (c) 2008-2011, OFFIS e.V. and ICSMED AG, Oldenburg, Germany
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  Source file for class DRTRTReferencedSeriesSequence
 *
 *  Generated automatically from DICOM PS 3.3-2007
 *  File created on 2011-08-23 18:49:38 
 *
 */


#include "dcmtk/config/osconfig.h"     /* make sure OS specific configuration is included first */

#include "dcmtk/dcmrt/drtrses.h"


// --- item class ---

DRTRTReferencedSeriesSequence::Item::Item(const OFBool emptyDefaultItem)
  : EmptyDefaultItem(emptyDefaultItem),
    ContourImageSequence(emptyDefaultItem /*emptyDefaultSequence*/),
    SeriesInstanceUID(DCM_SeriesInstanceUID)
{
}


DRTRTReferencedSeriesSequence::Item::Item(const Item &copy)
  : EmptyDefaultItem(copy.EmptyDefaultItem),
    ContourImageSequence(copy.ContourImageSequence),
    SeriesInstanceUID(copy.SeriesInstanceUID)
{
}


DRTRTReferencedSeriesSequence::Item::~Item()
{
}


DRTRTReferencedSeriesSequence::Item &DRTRTReferencedSeriesSequence::Item::operator=(const Item &copy)
{
    if (this != &copy)
    {
        EmptyDefaultItem = copy.EmptyDefaultItem;
        ContourImageSequence = copy.ContourImageSequence;
        SeriesInstanceUID = copy.SeriesInstanceUID;
    }
    return *this;
}


void DRTRTReferencedSeriesSequence::Item::clear()
{
    if (!EmptyDefaultItem)
    {
        /* clear all DICOM attributes */
        SeriesInstanceUID.clear();
        ContourImageSequence.clear();
    }
}


OFBool DRTRTReferencedSeriesSequence::Item::isEmpty()
{
    return SeriesInstanceUID.isEmpty() &&
           ContourImageSequence.isEmpty();
}


OFBool DRTRTReferencedSeriesSequence::Item::isValid() const
{
    return !EmptyDefaultItem;
}


OFCondition DRTRTReferencedSeriesSequence::Item::read(DcmItem &item)
{
    OFCondition result = EC_IllegalCall;
    if (!EmptyDefaultItem)
    {
        /* re-initialize object */
        clear();
        getAndCheckElementFromDataset(item, SeriesInstanceUID, "1", "1C", "RTReferencedSeriesSequence");
        ContourImageSequence.read(item, "1-n", "1C", "RTReferencedSeriesSequence");
        result = EC_Normal;
    }
    return result;
}


OFCondition DRTRTReferencedSeriesSequence::Item::write(DcmItem &item)
{
    OFCondition result = EC_IllegalCall;
    if (!EmptyDefaultItem)
    {
        result = EC_Normal;
        addElementToDataset(result, item, new DcmUniqueIdentifier(SeriesInstanceUID), "1", "1C", "RTReferencedSeriesSequence");
        if (result.good()) result = ContourImageSequence.write(item, "1-n", "1C", "RTReferencedSeriesSequence");
    }
    return result;
}


OFCondition DRTRTReferencedSeriesSequence::Item::getSeriesInstanceUID(OFString &value, const signed long pos) const
{
    if (EmptyDefaultItem)
        return EC_IllegalCall;
    else
        return getStringValueFromElement(SeriesInstanceUID, value, pos);
}


OFCondition DRTRTReferencedSeriesSequence::Item::setSeriesInstanceUID(const OFString &value, const OFBool check)
{
    OFCondition result = EC_IllegalCall;
    if (!EmptyDefaultItem)
    {
        result = (check) ? DcmUniqueIdentifier::checkStringValue(value, "1") : EC_Normal;
        if (result.good())
            result = SeriesInstanceUID.putString(value.c_str());
    }
    return result;
}


// --- sequence class ---

DRTRTReferencedSeriesSequence::DRTRTReferencedSeriesSequence(const OFBool emptyDefaultSequence)
  : EmptyDefaultSequence(emptyDefaultSequence),
    SequenceOfItems(),
    CurrentItem(),
    EmptyItem(OFTrue /*emptyDefaultItem*/)
{
    CurrentItem = SequenceOfItems.end();
}


DRTRTReferencedSeriesSequence::DRTRTReferencedSeriesSequence(const DRTRTReferencedSeriesSequence &copy)
  : EmptyDefaultSequence(copy.EmptyDefaultSequence),
    SequenceOfItems(),
    CurrentItem(),
    EmptyItem(OFTrue /*emptyDefaultItem*/)
{
    /* create a copy of the internal sequence of items */
    Item *item = NULL;
    OFListConstIterator(Item *) current = copy.SequenceOfItems.begin();
    const OFListConstIterator(Item *) last = copy.SequenceOfItems.end();
    while (current != last)
    {
        item = new Item(**current);
        if (item != NULL)
        {
            SequenceOfItems.push_back(item);
        } else {
            /* memory exhausted, there is nothing we can do about it */
            break;
        }
        ++current;
    }
    CurrentItem = SequenceOfItems.begin();
}


DRTRTReferencedSeriesSequence &DRTRTReferencedSeriesSequence::operator=(const DRTRTReferencedSeriesSequence &copy)
{
    if (this != &copy)
    {
        clear();
        EmptyDefaultSequence = copy.EmptyDefaultSequence;
        /* create a copy of the internal sequence of items */
        Item *item = NULL;
        OFListConstIterator(Item *) current = copy.SequenceOfItems.begin();
        const OFListConstIterator(Item *) last = copy.SequenceOfItems.end();
        while (current != last)
        {
            item = new Item(**current);
            if (item != NULL)
            {
                SequenceOfItems.push_back(item);
            } else {
                /* memory exhausted, there is nothing we can do about it */
                break;
            }
            ++current;
        }
        CurrentItem = SequenceOfItems.begin();
    }
    return *this;
}


DRTRTReferencedSeriesSequence::~DRTRTReferencedSeriesSequence()
{
    clear();
}


void DRTRTReferencedSeriesSequence::clear()
{
    if (!EmptyDefaultSequence)
    {
        CurrentItem = SequenceOfItems.begin();
        const OFListConstIterator(Item *) last = SequenceOfItems.end();
        /* delete all items and free memory */
        while (CurrentItem != last)
        {
            delete (*CurrentItem);
            CurrentItem = SequenceOfItems.erase(CurrentItem);
        }
        /* make sure that the list is empty */
        SequenceOfItems.clear();
        CurrentItem = SequenceOfItems.end();
    }
}


OFBool DRTRTReferencedSeriesSequence::isEmpty()
{
    return SequenceOfItems.empty();
}


OFBool DRTRTReferencedSeriesSequence::isValid() const
{
    return !EmptyDefaultSequence;
}


unsigned long DRTRTReferencedSeriesSequence::getNumberOfItems() const
{
    return SequenceOfItems.size();
}


OFCondition DRTRTReferencedSeriesSequence::gotoFirstItem()
{
    OFCondition result = EC_IllegalCall;
    if (!SequenceOfItems.empty())
    {
        CurrentItem = SequenceOfItems.begin();
        result = EC_Normal;
    }
    return result;
}


OFCondition DRTRTReferencedSeriesSequence::gotoNextItem()
{
    OFCondition result = EC_IllegalCall;
    if (CurrentItem != SequenceOfItems.end())
    {
        ++CurrentItem;
        result = EC_Normal;
    }
    return result;
}


OFCondition DRTRTReferencedSeriesSequence::gotoItem(const unsigned long num, OFListIterator(Item *) &iterator)
{
    OFCondition result = EC_IllegalCall;
    if (!SequenceOfItems.empty())
    {
        unsigned long idx = num + 1;
        iterator = SequenceOfItems.begin();
        const OFListConstIterator(Item *) last = SequenceOfItems.end();
        while ((--idx > 0) && (iterator != last))
            ++iterator;
        /* specified list item found? */
        if ((idx == 0) && (iterator != last))
            result = EC_Normal;
        else
            result = EC_IllegalParameter;
    }
    return result;
}


OFCondition DRTRTReferencedSeriesSequence::gotoItem(const unsigned long num, OFListConstIterator(Item *) &iterator) const
{
    OFCondition result = EC_IllegalCall;
    if (!SequenceOfItems.empty())
    {
        unsigned long idx = num + 1;
        iterator = SequenceOfItems.begin();
        const OFListConstIterator(Item *) last = SequenceOfItems.end();
        while ((--idx > 0) && (iterator != last))
            ++iterator;
        /* specified list item found? */
        if ((idx == 0) && (iterator != last))
            result = EC_Normal;
        else
            result = EC_IllegalParameter;
    }
    return result;
}


OFCondition DRTRTReferencedSeriesSequence::gotoItem(const unsigned long num)
{
    return gotoItem(num, CurrentItem);
}


OFCondition DRTRTReferencedSeriesSequence::getCurrentItem(Item *&item) const
{
    OFCondition result = EC_IllegalCall;
    if (CurrentItem != SequenceOfItems.end())
    {
        item = *CurrentItem;
        result = EC_Normal;
    }
    return result;
}


DRTRTReferencedSeriesSequence::Item &DRTRTReferencedSeriesSequence::getCurrentItem()
{
    if (CurrentItem != SequenceOfItems.end())
        return **CurrentItem;
    else
        return EmptyItem;
}


const DRTRTReferencedSeriesSequence::Item &DRTRTReferencedSeriesSequence::getCurrentItem() const
{
    if (CurrentItem != SequenceOfItems.end())
        return **CurrentItem;
    else
        return EmptyItem;
}


OFCondition DRTRTReferencedSeriesSequence::getItem(const unsigned long num, Item *&item)
{
    OFListIterator(Item *) iterator;
    OFCondition result = gotoItem(num, iterator);
    if (result.good())
        item = *iterator;
    return result;
}


DRTRTReferencedSeriesSequence::Item &DRTRTReferencedSeriesSequence::getItem(const unsigned long num)
{
    OFListIterator(Item *) iterator;
    if (gotoItem(num, iterator).good())
        return **iterator;
    else
        return EmptyItem;
}


const DRTRTReferencedSeriesSequence::Item &DRTRTReferencedSeriesSequence::getItem(const unsigned long num) const
{
    OFListConstIterator(Item *) iterator;
    if (gotoItem(num, iterator).good())
        return **iterator;
    else
        return EmptyItem;
}


DRTRTReferencedSeriesSequence::Item &DRTRTReferencedSeriesSequence::operator[](const unsigned long num)
{
    return getItem(num);
}


const DRTRTReferencedSeriesSequence::Item &DRTRTReferencedSeriesSequence::operator[](const unsigned long num) const
{
    return getItem(num);
}


OFCondition DRTRTReferencedSeriesSequence::addItem(Item *&item)
{
    OFCondition result = EC_IllegalCall;
    if (!EmptyDefaultSequence)
    {
        item = new Item();
        if (item != NULL)
        {
            SequenceOfItems.push_back(item);
            result = EC_Normal;
        } else
            result = EC_MemoryExhausted;
    }
    return result;
}


OFCondition DRTRTReferencedSeriesSequence::insertItem(const unsigned long pos, Item *&item)
{
    OFCondition result = EC_IllegalCall;
    if (!EmptyDefaultSequence)
    {
        OFListIterator(Item *) iterator;
        result = gotoItem(pos, iterator);
        if (result.good())
        {
            item = new Item();
            if (item != NULL)
            {
                SequenceOfItems.insert(iterator, 1, item);
                result = EC_Normal;
            } else
                result = EC_MemoryExhausted;
        } else
            result = addItem(item);
    }
    return result;
}


OFCondition DRTRTReferencedSeriesSequence::removeItem(const unsigned long pos)
{
    OFCondition result = EC_IllegalCall;
    if (!EmptyDefaultSequence)
    {
        OFListIterator(Item *) iterator;
        if (gotoItem(pos, iterator).good())
        {
            delete *iterator;
            iterator = SequenceOfItems.erase(iterator);
            result = EC_Normal;
        } else
            result = EC_IllegalParameter;
    }
    return result;
}


OFCondition DRTRTReferencedSeriesSequence::read(DcmItem &dataset,
                                                const OFString &card,
                                                const OFString &type,
                                                const char *moduleName)
{
    OFCondition result = EC_IllegalCall;
    if (!EmptyDefaultSequence)
    {
        /* re-initialize object */
        clear();
        /* retrieve sequence element from dataset */
        DcmSequenceOfItems *sequence;
        result = dataset.findAndGetSequence(DCM_RTReferencedSeriesSequence, sequence);
        if (sequence != NULL)
        {
            if (checkElementValue(*sequence, card, type, result, moduleName))
            {
                DcmStack stack;
                OFBool first = OFTrue;
                /* iterate over all sequence items */
                while (result.good() && sequence->nextObject(stack, first /*intoSub*/).good())
                {
                    DcmItem *ditem = OFstatic_cast(DcmItem *, stack.top());
                    if (ditem != NULL)
                    {
                        Item *item = new Item();
                        if (item != NULL)
                        {
                            result = item->read(*ditem);
                            if (result.good())
                            {
                                /* append new item to the end of the list */
                                SequenceOfItems.push_back(item);
                                first = OFFalse;
                            }
                        } else
                            result = EC_MemoryExhausted;
                    } else
                        result = EC_CorruptedData;
                }
            }
        } else {
            DcmSequenceOfItems element(DCM_RTReferencedSeriesSequence);
            checkElementValue(element, card, type, result, moduleName);
        }
    }
    return result;
}


OFCondition DRTRTReferencedSeriesSequence::write(DcmItem &dataset,
                                                 const OFString &card,
                                                 const OFString &type,
                                                 const char *moduleName)
{
    OFCondition result = EC_IllegalCall;
    if (!EmptyDefaultSequence)
    {
        result = EC_MemoryExhausted;
        DcmSequenceOfItems *sequence = new DcmSequenceOfItems(DCM_RTReferencedSeriesSequence);
        if (sequence != NULL)
        {
            result = EC_Normal;
            /* an empty optional sequence is not written */
            if ((type == "2") || !SequenceOfItems.empty())
            {
                OFListIterator(Item *) iterator = SequenceOfItems.begin();
                const OFListConstIterator(Item *) last = SequenceOfItems.end();
                /* iterate over all sequence items */
                while (result.good() && (iterator != last))
                {
                    DcmItem *item = new DcmItem();
                    if (item != NULL)
                    {
                        /* append new item to the end of the sequence */
                        result = sequence->append(item);
                        if (result.good())
                        {
                            result = (*iterator)->write(*item);
                            ++iterator;
                        } else
                            delete item;
                    } else
                        result = EC_MemoryExhausted;
                }
                if (result.good())
                {
                    /* insert sequence element into the dataset */
                    result = dataset.insert(sequence, OFTrue /*replaceOld*/);
                }
                if (DCM_dcmrtLogger.isEnabledFor(OFLogger::WARN_LOG_LEVEL))
                    checkElementValue(*sequence, card, type, result, moduleName);
                if (result.good())
                {
                    /* forget reference to sequence object (avoid deletion below) */
                    sequence = NULL;
                }
            }
            else if (type == "1")
            {
                /* empty type 1 sequence not allowed */
                result = RT_EC_InvalidValue;
                if (DCM_dcmrtLogger.isEnabledFor(OFLogger::WARN_LOG_LEVEL))
                    checkElementValue(*sequence, card, type, result, moduleName);
            }
            /* delete sequence (if not inserted into the dataset) */
            delete sequence;
        }
    }
    return result;
}


// end of source file
