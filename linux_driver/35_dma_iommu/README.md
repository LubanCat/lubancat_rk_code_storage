# dma_iommu

运行`make`命令后，将会有一个模块：

* dma_iommu.ko

加载驱动程序和内核调试信息：

1. dma_iommu.ko

```bash
# insmod dma_iommu.ko

   [   57.792710] DMA memory allocated:
   [   57.792741]   CPU Virtual address: 0x0000000006f36ce1
   [   57.792748]   Device IOVA address: 0x000000010c66e000
   [   57.792837] char device major=234, minor=0


# echo -n "hello world!" > /dev/dma_iommu

   [  128.850827] dma-iommu-test dma-iommu-test: Written to DMA memory: "hello world!"

# cat /dev/dma_iommu

   hello world!
```