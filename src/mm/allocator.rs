// Copyright(c) The Maintainers of Nanvix.
// Licensed under the MIT License.

//==================================================================================================
// Imports
//==================================================================================================

extern crate alloc;

use ::alloc::alloc::{
    AllocError,
    GlobalAlloc,
    Layout,
};
use ::core::ptr;
use ::error::{
    Error,
    ErrorCode,
};
use ::slab::Slab;

//==================================================================================================
//  Structures
//==================================================================================================

struct SlabAllocator;

///
/// # Description
///
/// Sizes of slab supported by the allocator.
///
#[derive(Copy, Clone)]
enum SlabSize {
    /// 8 bytes slab.
    Slab8 = 8,
    /// 16 bytes slab.
    Slab16 = 16,
    /// 32 bytes slab.
    Slab32 = 32,
    /// 64 bytes slab.
    Slab64 = 64,
    /// 128 bytes slab.
    Slab128 = 128,
    /// 256 bytes slab.
    Slab256 = 256,
    /// 512 bytes slab.
    Slab512 = 512,
    /// 1024 bytes slab.
    Slab1024 = 1024,
}

pub struct Heap {
    slab_8_bytes: Slab,
    slab_16_bytes: Slab,
    slab_32_bytes: Slab,
    slab_64_bytes: Slab,
    slab_128_bytes: Slab,
    slab_256_bytes: Slab,
    slab_512_bytes: Slab,
    slab_1024_bytes: Slab,
}

//==================================================================================================
// Global Variables
//==================================================================================================

static mut HEAP: Option<Heap> = None;

#[global_allocator]
static mut ALLOCATOR: SlabAllocator = SlabAllocator;

//==================================================================================================
// Implementations
//==================================================================================================

impl SlabSize {
    /// Number of slabs sizes.
    const COUNT: usize = 8;
    // Maximum slab size.
    const MAX_SIZE: usize = SlabSize::Slab1024 as usize;
}

impl Heap {
    /// Number of slabs provided.
    const NUM_OF_SLABS: usize = SlabSize::COUNT;
    // Number of slabs for each slab size.
    // NOTE: This must be at least 2, as one slab is used for space allocation.
    const SLAB_COUNT: usize = 2;
    /// Minimum slab size.
    const MIN_SLAB_SIZE: usize = Self::SLAB_COUNT * SlabSize::MAX_SIZE;
    /// Minimum heap size.
    pub const MIN_SIZE: usize = SlabSize::COUNT * Self::NUM_OF_SLABS * Self::MIN_SLAB_SIZE;

    ///
    /// # Description
    ///
    /// Initializes the heap.
    ///
    /// # Parameters
    ///
    /// - `addr` - Start address of the heap.
    /// - `size` - Size of the heap.
    ///
    /// # Returns
    ///
    /// Upon success, empty is returned. Upon failure, an error is returned instead
    ///
    pub unsafe fn init(addr: usize, size: usize) -> Result<(), Error> {
        // Check if the heap was already initialized.
        if unsafe { HEAP.is_some() } {
            return Err(Error::new(ErrorCode::ResourceBusy, "heap already initialized"));
        }

        HEAP = Some(Heap::from_raw_parts(addr, size)?);

        Ok(())
    }

    ///
    /// # Description
    ///
    /// Creates a new heap from an address and size.
    ///
    /// # Parameters
    ///
    /// - `addr` - Start address of the heap.
    /// - `size` - Size of the heap.
    ///
    /// # Returns
    ///
    /// Upon success, a new heap is returned. Upon failure, an error is returned instead.
    ///
    unsafe fn from_raw_parts(addr: usize, size: usize) -> Result<Heap, Error> {
        // Check if size is less than minimum heap size.
        if size < Self::MIN_SIZE {
            return Err(Error::new(
                ErrorCode::InvalidArgument,
                "heap size is less than minimum heap size",
            ));
        }

        // Check if size is not a multiple of heap size.
        if size % Self::MIN_SIZE != 0 {
            return Err(Error::new(
                ErrorCode::InvalidArgument,
                "size is not a multiple of page size",
            ));
        }

        let heap_start_addr: *mut u8 = addr as *mut u8;
        let slab_size: usize = size / Self::NUM_OF_SLABS;

        let slab_8_bytes: Slab =
            Slab::from_raw_parts(heap_start_addr, slab_size, SlabSize::Slab8 as usize)?;
        let slab_16_bytes: Slab = Slab::from_raw_parts(
            heap_start_addr.add(slab_size),
            slab_size,
            SlabSize::Slab16 as usize,
        )?;
        let slab_32_bytes: Slab = Slab::from_raw_parts(
            heap_start_addr.add(2 * slab_size),
            slab_size,
            SlabSize::Slab32 as usize,
        )?;
        let slab_64_bytes: Slab = Slab::from_raw_parts(
            heap_start_addr.add(3 * slab_size),
            slab_size,
            SlabSize::Slab64 as usize,
        )?;
        let slab_128_bytes: Slab = Slab::from_raw_parts(
            heap_start_addr.add(4 * slab_size),
            slab_size,
            SlabSize::Slab128 as usize,
        )?;
        let slab_256_bytes: Slab = Slab::from_raw_parts(
            heap_start_addr.add(5 * slab_size),
            slab_size,
            SlabSize::Slab256 as usize,
        )?;
        let slab_512_bytes: Slab = Slab::from_raw_parts(
            heap_start_addr.add(6 * slab_size),
            slab_size,
            SlabSize::Slab512 as usize,
        )?;
        let slab_4096_bytes: Slab = Slab::from_raw_parts(
            heap_start_addr.add(7 * slab_size),
            slab_size,
            SlabSize::Slab1024 as usize,
        )?;

        Ok(Heap {
            slab_8_bytes,
            slab_16_bytes,
            slab_32_bytes,
            slab_64_bytes,
            slab_128_bytes,
            slab_256_bytes,
            slab_512_bytes,
            slab_1024_bytes: slab_4096_bytes,
        })
    }

    unsafe fn allocate(&mut self, layout: Layout) -> Result<*mut u8, AllocError> {
        match Heap::layout_to_allocator(&layout)? {
            SlabSize::Slab8 => self.slab_8_bytes.allocate().map_err(|_| AllocError),
            SlabSize::Slab16 => self.slab_16_bytes.allocate().map_err(|_| AllocError),
            SlabSize::Slab32 => self.slab_32_bytes.allocate().map_err(|_| AllocError),
            SlabSize::Slab64 => self.slab_64_bytes.allocate().map_err(|_| AllocError),
            SlabSize::Slab128 => self.slab_128_bytes.allocate().map_err(|_| AllocError),
            SlabSize::Slab256 => self.slab_256_bytes.allocate().map_err(|_| AllocError),
            SlabSize::Slab512 => self.slab_512_bytes.allocate().map_err(|_| AllocError),
            SlabSize::Slab1024 => self.slab_1024_bytes.allocate().map_err(|_| AllocError),
        }
    }

    unsafe fn deallocate(&mut self, ptr: *mut u8, layout: Layout) -> Result<(), AllocError> {
        match Heap::layout_to_allocator(&layout)? {
            SlabSize::Slab8 => self.slab_8_bytes.deallocate(ptr).map_err(|_| AllocError),
            SlabSize::Slab16 => self.slab_16_bytes.deallocate(ptr).map_err(|_| AllocError),
            SlabSize::Slab32 => self.slab_32_bytes.deallocate(ptr).map_err(|_| AllocError),
            SlabSize::Slab64 => self.slab_64_bytes.deallocate(ptr).map_err(|_| AllocError),
            SlabSize::Slab128 => self.slab_128_bytes.deallocate(ptr).map_err(|_| AllocError),
            SlabSize::Slab256 => self.slab_256_bytes.deallocate(ptr).map_err(|_| AllocError),
            SlabSize::Slab512 => self.slab_512_bytes.deallocate(ptr).map_err(|_| AllocError),
            SlabSize::Slab1024 => self.slab_1024_bytes.deallocate(ptr).map_err(|_| AllocError),
        }
    }

    fn layout_to_allocator(layout: &Layout) -> Result<SlabSize, AllocError> {
        match layout.size() {
            1..=8 => Ok(SlabSize::Slab8),
            9..=16 => Ok(SlabSize::Slab16),
            17..=32 => Ok(SlabSize::Slab32),
            33..=64 => Ok(SlabSize::Slab64),
            65..=128 => Ok(SlabSize::Slab128),
            129..=256 => Ok(SlabSize::Slab256),
            257..=512 => Ok(SlabSize::Slab512),
            513..=1024 => Ok(SlabSize::Slab1024),
            _ => Err(AllocError),
        }
    }
}

//==================================================================================================
// Standalone Functions
//==================================================================================================

unsafe impl GlobalAlloc for SlabAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let heap = ptr::addr_of_mut!(HEAP);
        if let Some(heap) = &mut *heap {
            match heap.allocate(layout) {
                Ok(ptr) => ptr,
                Err(_) => {
                    // Allocation failed.
                    core::ptr::null_mut()
                },
            }
        } else {
            // Heap is not initialized.
            core::ptr::null_mut()
        }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        let heap = ptr::addr_of_mut!(HEAP);
        if let Some(heap) = &mut *heap {
            if let Err(_) = heap.deallocate(ptr, layout) {
                // Deallocation failed.
            }
        }
    }
}
