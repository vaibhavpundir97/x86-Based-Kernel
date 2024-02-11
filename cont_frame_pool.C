/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INITIALIZATION OF STATIC VARIABLES FOR CLASS C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool * ContFramePool::head = nullptr;
ContFramePool * ContFramePool::list = nullptr;

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no) {
    unsigned int bitmap_index = _frame_no / 4;
    unsigned char mask = 0x3 << (_frame_no % 4) * 2;
    
    switch((bitmap[bitmap_index] & mask) >> (_frame_no % 4) * 2) {
        case 0x0:
        return FrameState::Free;
        case 0x1:
        return FrameState::Used;
        case 0x2:
        return FrameState::HoS;
        case 0x3:
        return FrameState::Inaccessible;
    }
    return FrameState::Free;
}

void ContFramePool::set_state(unsigned long _frame_no, FrameState _state) {
    unsigned int bitmap_index = _frame_no / 4;
    unsigned char mask = 0x3 << (_frame_no % 4) * 2;

    switch(_state) {
        case FrameState::Free:
        bitmap[bitmap_index] &= ~mask;
        break;
        case FrameState::Used:
        bitmap[bitmap_index] &= ~mask;
        bitmap[bitmap_index] |= 0x1 << (_frame_no % 4) * 2;
        break;
        case FrameState::HoS:
        bitmap[bitmap_index] &= ~mask;
        bitmap[bitmap_index] |= 0x2 << (_frame_no % 4) * 2;
        break;
        case FrameState::Inaccessible:
        bitmap[bitmap_index] &= ~mask;
        bitmap[bitmap_index] |= 0x3 << (_frame_no % 4) * 2;
        break;
    }
}

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!
    // Console::puts("ContframePool::Constructor not implemented!\n");
    // assert(false);
    base_frame_no = _base_frame_no;
    n_frames = _n_frames;
    n_free_frames = _n_frames;
    info_frame_no = _info_frame_no;

    bitmap = (unsigned char *) ((info_frame_no ? info_frame_no : base_frame_no) * FRAME_SIZE);

    for(unsigned long fno = 0; fno < n_frames; ++fno) {
        set_state(fno, FrameState::Free);
    }

    // Mark the first frame as being used if it is being used
    if(!info_frame_no) {
        set_state(0, FrameState::Used);
        --n_free_frames;
    }

    if(!ContFramePool::head)
        ContFramePool::head = this;
    else
        ContFramePool::list->next = this;

    ContFramePool::list = this;
    next = nullptr;

    Console::puts("Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    // Console::puts("ContframePool::get_frames not implemented!\n");
    // assert(false);
    assert(_n_frames <= n_free_frames);
    // if(_n_frames > n_free_frames) {
    //     Console::puts("ContframePool::get_frames - not enough frames available!\n");
    //     return 0;
    // }

    unsigned long start;
    unsigned long num = 0;
    for(unsigned long n=0; n < n_frames; ++n) {
        if(get_state(n) == FrameState::Free) {
            if(!num)
                start = n;
            ++num;
            if(num == _n_frames)
                break;
        } else {
            num = 0;
        }
    }

    if(num != _n_frames) {
        Console::puts("ContframePool::get_frames - not enough contiguous frames available!\n");
        return 0;
    }

    for(unsigned long id = 0; id < _n_frames; ++id) {
        set_state(id + start, id == 0 ? FrameState::HoS : FrameState::Used);
    }
    n_free_frames -= _n_frames;

    return start + base_frame_no;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    // Console::puts("ContframePool::mark_inaccessible not implemented!\n");
    // assert(false);
    assert(_base_frame_no >= base_frame_no);

	for(unsigned long fno = _base_frame_no; fno < _base_frame_no + _n_frames; ++fno) {
        assert(fno <= base_frame_no + n_frames && get_state(fno - base_frame_no) == FrameState::Free);

        // if(fno > base_frame_no + n_frames) {
        //     Console::puts("ContframePool::mark_inaccessible - out of range!\n");
        //     return;
        // }
        // if(get_state(fno) == FrameState::Used) {
        //     Console::puts("ContframePool::mark_inaccessible - Frame already in use!\n");
        //     return;
        // }
        set_state(fno - base_frame_no, FrameState::Inaccessible);
        --n_free_frames;
    }
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!
    // Console::puts("ContframePool::release_frames not implemented!\n");
    // assert(false);
    ContFramePool * node = ContFramePool::head;
    for(; node != nullptr; node = node->next) {
        if(_first_frame_no >= node->base_frame_no && _first_frame_no < node->base_frame_no + node->n_frames) {
            unsigned long n = _first_frame_no - node->base_frame_no;
            if(node->get_state(n) == FrameState::HoS) {
                for(unsigned long fno = _first_frame_no; fno < _first_frame_no + node->n_frames; ++fno) {
                    node->set_state(fno - node->base_frame_no, FrameState::Free);
                    node->n_free_frames -= 1;
                }
            } else {
                Console::puts("ContFramePool::release_frames - {_first_frame_no} is not in HoS frame state!\n");
                assert(false);
            }
            break;
        }
    }
    if(!node) {
        Console::puts("ContFramePool::release_frames - {_first_frame_no} not found in any frame pools!\n");
        assert(false);
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    // Console::puts("ContframePool::need_info_frames not implemented!\n");
    // assert(false);
    return (_n_frames * 2) / (8 * 4 << 10) + (_n_frames * 2) % (8 * 4 << 10) ? 1 : 0;
}
