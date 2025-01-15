#import smbus
import smbus2
import time
# 对应的Bus位置也要修改
bus = smbus2.SMBus(6)
 #修改对应的地址位置
device_2906 = 0x60
device_2905_1 = 0x50
device_2905_2 = 0x51
#device_cdphy_tx = 0x28
#device_cdphy_rx_1 = 0x18
#device_cdphy_rx_2 = 0x19
reg_addr = 0x2007

device_cdphy_tx = 0x60
device_cdphy_rx_1 = 0x50
device_cdphy_rx_2 = 0x51

mpw_cdphy_tx_base = 0x0090
mpw_cdphy_rx_base = 0x00b0

def register_read(device_address, register_address, num_bytes_to_read=1):
        try:
            # Step 1: Write to the device, indicating which register you want to read
            write_msg = smbus2.i2c_msg.write(device_address, [register_address>>8, register_address & 0xFF])
            
            # Step 2: Prepare to read back a certain number of bytes from the device
            # Let's say we want to read back 2 bytes from the register
            read_msg = smbus2.i2c_msg.read(device_address, num_bytes_to_read)
            
            # Perform the write-then-read operation
            bus.i2c_rdwr(write_msg, read_msg)
            
            # Convert the read message to a list of byte values
            read_data = list(read_msg)
            
            #print(f"Data read from device: {read_data}")
            #for byte in read_data:
            #   print (hex(byte), end=' ')
            #print (end='\n')
        finally:
            pass 
        #    bus.close()
        return read_data

def register_write(device_address, register_address, data):
        try:
            # Step 1: Write to the device, indicating which register you want to read
            write_msg = smbus2.i2c_msg.write(device_address, [register_address>>8, register_address & 0xFF, data[0]])
            
            # Step 2: Prepare to read back a certain number of bytes from the device
            # Let's say we want to read back 2 bytes from the register
            write_msg2 = smbus2.i2c_msg.write(device_address, data)
            
            # Perform the write-then-read operation
            #bus.i2c_rdwr(write_msg, write_msg2)
            bus.i2c_rdwr(write_msg)
            
            
            #print(f"Data write to device: {data}")
        finally:
            pass
         #   bus.close()
         
def register_write16(device_address, register_address, data):
        try:
            # Step 1: Write to the device, indicating which register you want to read
            write_msg = smbus2.i2c_msg.write(device_address, [register_address>>8, register_address & 0xFF, data[1], data[0]])
            
            # Step 2: Prepare to read back a certain number of bytes from the device
            # Let's say we want to read back 2 bytes from the register
            write_msg2 = smbus2.i2c_msg.write(device_address, data)
            
            # Perform the write-then-read operation
            #bus.i2c_rdwr(write_msg, write_msg2)
            bus.i2c_rdwr(write_msg)
            
            
            #print(f"Data write to device: {data}")
        finally:
            pass
         #   bus.close()


def register_write_mod(device_address, register_address, mask, data):
        try:
            regval = register_read(device_address, register_address, 1)
            #print(regval)
            #print("---------1-----")
            regval[0] &= ~(mask)
            data &= mask
            data |= regval[0]
            #print(data)
            #print("---------2-----")
            register_write(device_address, register_address, [data,])
        finally:
            pass
            
    
def reg_mod(value, mask, data):
        regval = value
        regval &= ~(mask)
        data &= mask
        data |= regval
        return data
         
         
def va2906_cdphy():
        try:
            reg_addr=0x0100 
            reg_addr += mpw_cdphy_tx_base
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x15])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
        
            reg_addr=0x0800 
            reg_addr += mpw_cdphy_tx_base
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x3a])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0600 
            reg_addr += mpw_cdphy_tx_base
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x04])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x1800
            reg_addr += mpw_cdphy_tx_base
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x00])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x1900
            reg_addr += mpw_cdphy_tx_base
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x14])   #1Gbps
            #register_write16(device_cdphy_tx, reg_addr, [0x00,0x10])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x1a00
            reg_addr += mpw_cdphy_tx_base
            #register_write16(device_cdphy_tx, reg_addr, [0x00,0x20])   #1Gbps
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x40])    #500Mbps
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x1b00
            reg_addr += mpw_cdphy_tx_base
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x00])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0700
            reg_addr += mpw_cdphy_tx_base
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x11])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0002
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x01])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x1302
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            #register_write16(device_cdphy_tx, reg_addr, [0x00,0x01])
            #reg = register_read(device_cdphy_tx, reg_addr, 2)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x1402
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            #register_write16(device_cdphy_tx, reg_addr, [0x00,0x00])
            #reg = register_read(device_cdphy_tx, reg_addr, 2)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x1502
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            #register_write16(device_cdphy_tx, reg_addr, [0x00,0x05])
            #reg = register_read(device_cdphy_tx, reg_addr, 2)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x1602
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            #register_write16(device_cdphy_tx, reg_addr, [0x00,0x50])
            #reg = register_read(device_cdphy_tx, reg_addr, 2)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x1702
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            #register_write16(device_cdphy_tx, reg_addr, [0x00,0x00])
            #reg = register_read(device_cdphy_tx, reg_addr, 2)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x5302
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x10])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x4202
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x10])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x4302
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x06])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x4502
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x10])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x4402
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x06])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x4b02
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x04])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x4702
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x0a])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x5402
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x08])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x4802
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x10])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x4902
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x05])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x4a02
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x08])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x4c02
            reg_addr += mpw_cdphy_tx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x04])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            ##CDPHYTX0  接口上，需要clk0 ,d0 ,d1做P,N swap.
            reg_addr=0x1e90
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            regval = reg_mod(hex(reg[0]), 0x1<<5, 0x1<<5)
            register_write16(device_cdphy_tx, reg_addr, [0x00,regval])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0190
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            regval = reg_mod(hex(reg[0]), 0x1<<0, 0x1<<0)
            register_write16(device_cdphy_tx, reg_addr, [0x00,regval])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0290
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            regval = reg_mod(hex(reg[0]), 0x3<<0, 0x3<<0)
            register_write16(device_cdphy_tx, reg_addr, [0x00,regval])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x1e90
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            regval = reg_mod(hex(reg[0]), 0x1<<5, 0x0<<5)
            register_write16(device_cdphy_tx, reg_addr, [0x00,regval])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            ##
            
            reg_addr=0x0600
            reg_addr += mpw_cdphy_tx_base
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x06])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            time.sleep(1)
            reg_addr=0x0600
            reg_addr += mpw_cdphy_tx_base
            register_write16(device_cdphy_tx, reg_addr, [0x00,0x07])
            reg = register_read(device_cdphy_tx, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
        finally:
            pass
         
def va2906_cdphy_mpw():
        try:
        
            print("---------va2906_cdphy_mpw-------------")
            reg_addr=0x6c30
            register_write_mod(device_2906, reg_addr, 0x7<<3, 0x6<<3)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c2c
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c2d
            register_write(device_2906, reg_addr, [0x50,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c2e
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c2f
            register_write(device_2906, reg_addr, [0x03,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c31
            register_write(device_2906, reg_addr, [0x9f,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c32
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c33
            register_write(device_2906, reg_addr, [0xbe,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c34
            register_write(device_2906, reg_addr, [0xbc,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c35
            register_write(device_2906, reg_addr, [0x0,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c36
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c37
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c38
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x6c30
            register_write_mod(device_2906, reg_addr, 0x7<<3, 0x1<<3)
            time.sleep(1)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            print("---------2906--pll--lock----------")
            
            
            reg_addr=0x5d10
            register_write_mod(device_2906, reg_addr, 0x3<<0, 0x0<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5d31
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x9000
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print(hex(reg[0]), end='\n')
            
            reg_addr=0x9013
            register_write_mod(device_2906, reg_addr, 0x1f<<0, 0x1<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9015
            register_write_mod(device_2906, reg_addr, 0x1f<<0, 0x4<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9074
            register_write_mod(device_2906, reg_addr, 0x1<<2, 0x0<<2)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x913d
            register_write_mod(device_2906, reg_addr, 0x3<<4, 0x3<<4)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9053
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9042
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9043
            register_write(device_2906, reg_addr, [0x06,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9045
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9044
            register_write(device_2906, reg_addr, [0x06,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x904b
            register_write(device_2906, reg_addr, [0x04,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9047
            register_write(device_2906, reg_addr, [0x0a,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9054
            register_write(device_2906, reg_addr, [0x08,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9048
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9049
            register_write(device_2906, reg_addr, [0x05,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x904a
            register_write(device_2906, reg_addr, [0x08,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x904c
            register_write(device_2906, reg_addr, [0x04,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            ##CDPHYTX0  接口上，需要clk0 ,d0 ,d1做P,N swap.
            reg_addr=0x901e
            register_write_mod(device_2906, reg_addr, 0x1<<5, 0x1<<5)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            
            reg_addr=0x9402
            register_write_mod(device_2906, reg_addr, 0x3<<0, 0x3<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x901e
            register_write_mod(device_2906, reg_addr, 0x1<<5, 0x0<<5)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            ##
            
            
            reg_addr=0x6c26
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x8<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c26
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0xc<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9074
            register_write_mod(device_2906, reg_addr, 0x1<<1, 0x1<<1)
            time.sleep(1)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            print("---------va2906_cdphy_mpw-------end------")
            
        finally:
            pass
            
def va2906_vpg():
        try:
            reg_addr=0x5400
            register_write_mod(device_2906, reg_addr, 0xf<<4, 0x0<<4)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5400
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x1<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出yuv
            print("----------------10-------1---------")
            reg_addr=0x5016
            register_write(device_2906, reg_addr, [0xec,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5017
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5018
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5019
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501a
            register_write(device_2906, reg_addr, [0x4a,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501b
            register_write(device_2906, reg_addr, [0x61,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501c
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501d
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501e
            register_write(device_2906, reg_addr, [0x7e,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501f
            register_write(device_2906, reg_addr, [0x44,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5020
            register_write(device_2906, reg_addr, [0xcc,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5021
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5022
            register_write(device_2906, reg_addr, [0xde,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5023
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5024
            register_write(device_2906, reg_addr, [0x89,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5025
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5026
            register_write(device_2906, reg_addr, [0xa4,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5027
            register_write(device_2906, reg_addr, [0x30,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5028
            register_write(device_2906, reg_addr, [0x19,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5029
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502a
            register_write(device_2906, reg_addr, [0xb1,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502b
            register_write(device_2906, reg_addr, [0x9f,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502c
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502d
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502e
            register_write(device_2906, reg_addr, [0x1d,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502f
            register_write(device_2906, reg_addr, [0xf0,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5030
            register_write(device_2906, reg_addr, [0x77,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5031
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5032
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5033
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5034
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5035
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出彩条(va2906)
            #VPG配置(720p, 30fps, YUV422 8-bit)
            print("----------------10-------2---------")
            
            reg_addr=0x5001
            register_write(device_2906, reg_addr, [0x1e,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5004
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5005
            register_write(device_2906, reg_addr, [0x05,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5008
            register_write(device_2906, reg_addr, [0xb6,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5009
            register_write(device_2906, reg_addr, [0x03,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5010
            register_write(device_2906, reg_addr, [0x6e,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5011
            register_write(device_2906, reg_addr, [0x0c,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5006
            register_write(device_2906, reg_addr, [0xd0,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5007
            register_write(device_2906, reg_addr, [0x02,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5014
            register_write(device_2906, reg_addr, [0x0f,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5015
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5012
            register_write(device_2906, reg_addr, [0xee,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5013
            register_write(device_2906, reg_addr, [0x02,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5041
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5042
            register_write(device_2906, reg_addr, [0x0a,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5063
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5064
            register_write(device_2906, reg_addr, [0x0a,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5000
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
        finally:
            pass
 

def va2906_vpg_2():
        try:
            reg_addr=0x5400
            register_write_mod(device_2906, reg_addr, 0xf<<4, 0x0<<4)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5400
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x1<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出yuv
            print("----------------10-------1---------")
            reg_addr=0x5016
            register_write(device_2906, reg_addr, [0xec,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5017
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5018
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5019
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501a
            register_write(device_2906, reg_addr, [0x4a,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501b
            register_write(device_2906, reg_addr, [0x61,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501c
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501d
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501e
            register_write(device_2906, reg_addr, [0x7e,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501f
            register_write(device_2906, reg_addr, [0x44,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5020
            register_write(device_2906, reg_addr, [0xcc,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5021
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5022
            register_write(device_2906, reg_addr, [0xde,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5023
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5024
            register_write(device_2906, reg_addr, [0x89,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5025
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5026
            register_write(device_2906, reg_addr, [0xa4,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5027
            register_write(device_2906, reg_addr, [0x30,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5028
            register_write(device_2906, reg_addr, [0x19,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5029
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502a
            register_write(device_2906, reg_addr, [0xb1,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502b
            register_write(device_2906, reg_addr, [0x9f,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502c
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502d
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502e
            register_write(device_2906, reg_addr, [0x1d,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502f
            register_write(device_2906, reg_addr, [0xf0,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5030
            register_write(device_2906, reg_addr, [0x77,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5031
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5032
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5033
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5034
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5035
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出彩条(va2906)
            #VPG配置(640x480, 30fps, YUV422 8-bit)
            print("----------------10-------2---------")
            
            reg_addr=0x5001
            register_write(device_2906, reg_addr, [0x1e,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5004
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5005
            register_write(device_2906, reg_addr, [0x02,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5008
            register_write(device_2906, reg_addr, [0xc1,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5009
            register_write(device_2906, reg_addr, [0x05,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5010
            register_write(device_2906, reg_addr, [0xa3,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5011
            register_write(device_2906, reg_addr, [0x0e,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5006
            register_write(device_2906, reg_addr, [0xe0,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5007
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5014
            register_write(device_2906, reg_addr, [0x0f,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5015
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5012
            register_write(device_2906, reg_addr, [0xfe,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5013
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5041
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5042
            register_write(device_2906, reg_addr, [0x05,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5063
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5064
            register_write(device_2906, reg_addr, [0x05,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5000
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
        finally:
            pass 
            

def va2906_vpg_3():
        try:
            reg_addr=0x5400
            register_write_mod(device_2906, reg_addr, 0xf<<4, 0x0<<4)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5400
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x1<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出yuv
            print("----------------10-------1---------")
            reg_addr=0x5016
            register_write(device_2906, reg_addr, [0xec,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5017
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5018
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5019
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501a
            register_write(device_2906, reg_addr, [0x4a,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501b
            register_write(device_2906, reg_addr, [0x61,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501c
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501d
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501e
            register_write(device_2906, reg_addr, [0x7e,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x501f
            register_write(device_2906, reg_addr, [0x44,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5020
            register_write(device_2906, reg_addr, [0xcc,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5021
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5022
            register_write(device_2906, reg_addr, [0xde,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5023
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5024
            register_write(device_2906, reg_addr, [0x89,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5025
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5026
            register_write(device_2906, reg_addr, [0xa4,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5027
            register_write(device_2906, reg_addr, [0x30,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5028
            register_write(device_2906, reg_addr, [0x19,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5029
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502a
            register_write(device_2906, reg_addr, [0xb1,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502b
            register_write(device_2906, reg_addr, [0x9f,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502c
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502d
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502e
            register_write(device_2906, reg_addr, [0x1d,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x502f
            register_write(device_2906, reg_addr, [0xf0,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5030
            register_write(device_2906, reg_addr, [0x77,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5031
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5032
            register_write(device_2906, reg_addr, [0x10,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5033
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5034
            register_write(device_2906, reg_addr, [0x80,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5035
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出彩条(va2906)
            #VPG配置(720p, 10fps, YUV422 8-bit)
            print("----------------10-------2---------")
            
            reg_addr=0x5001
            register_write(device_2906, reg_addr, [0x1e,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5004
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5005
            register_write(device_2906, reg_addr, [0x05,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5008
            register_write(device_2906, reg_addr, [0x65,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5009
            register_write(device_2906, reg_addr, [0x0c,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5010
            register_write(device_2906, reg_addr, [0xcb,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5011
            register_write(device_2906, reg_addr, [0x1d,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5006
            register_write(device_2906, reg_addr, [0xd0,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5007
            register_write(device_2906, reg_addr, [0x02,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5012
            register_write(device_2906, reg_addr, [0xee,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5013
            register_write(device_2906, reg_addr, [0x02,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5014
            register_write(device_2906, reg_addr, [0x0f,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5015
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5041
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5042
            register_write(device_2906, reg_addr, [0x0a,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5063
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5064
            register_write(device_2906, reg_addr, [0x0a,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5000
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
        finally:
            pass
            
            
def va2905_cdphy_1():
        try:
            reg_addr=0x0100
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_1, reg_addr, [0x00,0x15])
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
        
            reg_addr=0x0800
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_1, reg_addr, [0x00,0x3a])
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0600
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_1, reg_addr, [0x00,0x00])
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0700
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_1, reg_addr, [0x00,0x11])
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0002
            reg_addr += mpw_cdphy_rx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_rx_1, reg_addr, [0x00,0x01])
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x3102
            reg_addr += mpw_cdphy_rx_base
            reg_addr -= 0x02
            register_write16(device_cdphy_rx_1, reg_addr, [0x00,0x14])  #5647-0x14
            #register_write16(device_cdphy_rx_1, reg_addr, [0x00,0x12])  
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            
            ##2905  clk  PN swap
            reg_addr=0x01b0
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            regval = reg_mod(hex(reg[0]), 0x1<<0, 0x1<<0)
            register_write16(device_cdphy_rx_1, reg_addr, [0x00,regval])
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            ##
            
            reg_addr=0x0600
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_1, reg_addr, [0x00,0x02])
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0xffff
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_1, reg_addr, [0x00,0x02])
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            time.sleep(1)
            
            reg_addr=0x0600
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_1, reg_addr, [0x00,0x03])
            reg = register_read(device_cdphy_rx_1, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
        
            
        finally:
            pass
            
   
def va2905_cdphy_1_mpw():
        try:
        
            print("---------va2905_cdphy_1_mpw---------")
            reg_addr=0xb000
            register_write(device_2905_1, reg_addr, [0x01,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
        
            reg_addr=0xb013
            register_write_mod(device_2905_1, reg_addr, 0x1f<<0, 0x1<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xb015
            register_write_mod(device_2905_1, reg_addr, 0x1f<<0, 0x4<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xb131
            register_write_mod(device_2905_1, reg_addr, 0xf<<0, 0x0<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xb13d
            register_write(device_2905_1, reg_addr, [0x25,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xb136
            register_write_mod(device_2905_1, reg_addr, 0x1<<6, 0x0<<6)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xb13e
            register_write_mod(device_2905_1, reg_addr, 0x1<<3, 0x1<<3)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xb007
            register_write_mod(device_2905_1, reg_addr, 0x3<<6, 0x2<<6)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xb031
            register_write(device_2905_1, reg_addr, [0x12,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            
            reg_addr=0xb001
            register_write_mod(device_2905_1, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            
            time.sleep(1)
            
            reg_addr=0xb074
            #register_write_mod(device_2905_1, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            print("---------va2905_cdphy_1_mpw---end------")
        finally:
            pass

            
def va2905_cdphy_2():
        try:
            reg_addr=0x0100
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_2, reg_addr, [0x00,0x15])
            reg = register_read(device_cdphy_rx_2, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
        
            reg_addr=0x0800
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_2, reg_addr, [0x00,0x3a])
            reg = register_read(device_cdphy_rx_2, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0600
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_2, reg_addr, [0x00,0x00])
            reg = register_read(device_cdphy_rx_2, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0700
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_2, reg_addr, [0x00,0x11])
            reg = register_read(device_cdphy_rx_2, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0002
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_2, reg_addr, [0x00,0x01])
            reg = register_read(device_cdphy_rx_2, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x3102
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_2, reg_addr, [0x00,0x14])  #5647-0x14
            #register_write16(device_cdphy_rx_2, reg_addr, [0x00,0x12])  
            reg = register_read(device_cdphy_rx_2, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0x0600
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_2, reg_addr, [0x00,0x02])
            reg = register_read(device_cdphy_rx_2, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
            reg_addr=0xffff
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_2, reg_addr, [0x00,0x02])
            reg = register_read(device_cdphy_rx_2, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            time.sleep(1)
            
            reg_addr=0x0600
            reg_addr += mpw_cdphy_rx_base
            register_write16(device_cdphy_rx_2, reg_addr, [0x00,0x03])
            reg = register_read(device_cdphy_rx_2, reg_addr, 2)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]),hex(reg[1]), end='\n')
            
        
            
        finally:
            pass

            
def va2905_vpg_1():
        try:
            reg_addr=0x902b
            register_write_mod(device_2905_1, reg_addr, 0x3<<2, 0x2<<2)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4000
            register_write_mod(device_2905_1, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出彩条(va2905)
            #VPG配置(720p, 30fps, YUV422 8-bit)
            print("----------------10----------------")
            reg_addr=0x1001
            register_write(device_2905_1, reg_addr, [0x1e,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1004
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1005
            register_write(device_2905_1, reg_addr, [0x05,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1008
            register_write(device_2905_1, reg_addr, [0xb6,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1009
            register_write(device_2905_1, reg_addr, [0x03,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1010
            register_write(device_2905_1, reg_addr, [0x6e,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1011
            register_write(device_2905_1, reg_addr, [0x0c,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1006
            register_write(device_2905_1, reg_addr, [0xd0,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1007
            register_write(device_2905_1, reg_addr, [0x02,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1014
            register_write(device_2905_1, reg_addr, [0x0f,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1015
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1012
            register_write(device_2905_1, reg_addr, [0xee,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1013
            register_write(device_2905_1, reg_addr, [0x02,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1041
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1042
            register_write(device_2905_1, reg_addr, [0x0a,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1063
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1064
            register_write(device_2905_1, reg_addr, [0x0a,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1000
            register_write(device_2905_1, reg_addr, [0x01,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')    
            
        
            
        finally:
            pass
            
 
def va2905_vpg_3():
        try:
            reg_addr=0x902b
            register_write_mod(device_2905_1, reg_addr, 0x3<<2, 0x2<<2)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4000
            register_write_mod(device_2905_1, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出yuv
            print("----------------10-------1---------")
            reg_addr=0x1016
            register_write(device_2905_1, reg_addr, [0xec,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1017
            register_write(device_2905_1, reg_addr, [0x80,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1018
            register_write(device_2905_1, reg_addr, [0x80,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1019
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101a
            register_write(device_2905_1, reg_addr, [0x4a,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101b
            register_write(device_2905_1, reg_addr, [0x61,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101c
            register_write(device_2905_1, reg_addr, [0x80,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101d
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101e
            register_write(device_2905_1, reg_addr, [0x7e,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101f
            register_write(device_2905_1, reg_addr, [0x44,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1020
            register_write(device_2905_1, reg_addr, [0xcc,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1021
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1022
            register_write(device_2905_1, reg_addr, [0xde,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1023
            register_write(device_2905_1, reg_addr, [0x10,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1024
            register_write(device_2905_1, reg_addr, [0x89,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1025
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1026
            register_write(device_2905_1, reg_addr, [0xa4,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1027
            register_write(device_2905_1, reg_addr, [0x30,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1028
            register_write(device_2905_1, reg_addr, [0x19,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1029
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102a
            register_write(device_2905_1, reg_addr, [0xb1,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102b
            register_write(device_2905_1, reg_addr, [0x9f,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102c
            register_write(device_2905_1, reg_addr, [0x10,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102d
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102e
            register_write(device_2905_1, reg_addr, [0x1d,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102f
            register_write(device_2905_1, reg_addr, [0xf0,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1030
            register_write(device_2905_1, reg_addr, [0x77,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1031
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1032
            register_write(device_2905_1, reg_addr, [0x10,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1033
            register_write(device_2905_1, reg_addr, [0x80,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1034
            register_write(device_2905_1, reg_addr, [0x80,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1035
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出彩条(va2905)
            #VPG配置(720p, 10fps, YUV422 8-bit)
            print("----------------10----------------")
            reg_addr=0x1001
            register_write(device_2905_1, reg_addr, [0x1e,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1004
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1005
            register_write(device_2905_1, reg_addr, [0x05,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1008
            #register_write(device_2905_1, reg_addr, [0xb6,])
            register_write(device_2905_1, reg_addr, [0xc4,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1009
            #register_write(device_2905_1, reg_addr, [0x03,])
            register_write(device_2905_1, reg_addr, [0x0b,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1010
            #register_write(device_2905_1, reg_addr, [0x6e,])
            register_write(device_2905_1, reg_addr, [0xca,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1011
            #register_write(device_2905_1, reg_addr, [0x0c,])
            register_write(device_2905_1, reg_addr, [0x1d,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1006
            register_write(device_2905_1, reg_addr, [0xd0,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1007
            register_write(device_2905_1, reg_addr, [0x02,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1014
            register_write(device_2905_1, reg_addr, [0x0f,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1015
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1012
            register_write(device_2905_1, reg_addr, [0xee,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1013
            register_write(device_2905_1, reg_addr, [0x02,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1041
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1042
            register_write(device_2905_1, reg_addr, [0x0a,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1063
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1064
            register_write(device_2905_1, reg_addr, [0x0a,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1000
            register_write(device_2905_1, reg_addr, [0x01,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')    
            
            
        finally:
            pass

def va2905_vpg_4():
        try:
            reg_addr=0x902b
            register_write_mod(device_2905_1, reg_addr, 0x3<<2, 0x2<<2)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4000
            register_write_mod(device_2905_1, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出yuv
            print("----------------10-------1---------")
            reg_addr=0x1016
            register_write(device_2905_1, reg_addr, [0xec,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1017
            register_write(device_2905_1, reg_addr, [0x80,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1018
            register_write(device_2905_1, reg_addr, [0x80,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1019
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101a
            register_write(device_2905_1, reg_addr, [0x4a,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101b
            register_write(device_2905_1, reg_addr, [0x61,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101c
            register_write(device_2905_1, reg_addr, [0x80,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101d
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101e
            register_write(device_2905_1, reg_addr, [0x7e,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101f
            register_write(device_2905_1, reg_addr, [0x44,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1020
            register_write(device_2905_1, reg_addr, [0xcc,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1021
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1022
            register_write(device_2905_1, reg_addr, [0xde,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1023
            register_write(device_2905_1, reg_addr, [0x10,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1024
            register_write(device_2905_1, reg_addr, [0x89,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1025
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1026
            register_write(device_2905_1, reg_addr, [0xa4,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1027
            register_write(device_2905_1, reg_addr, [0x30,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1028
            register_write(device_2905_1, reg_addr, [0x19,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1029
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102a
            register_write(device_2905_1, reg_addr, [0xb1,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102b
            register_write(device_2905_1, reg_addr, [0x9f,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102c
            register_write(device_2905_1, reg_addr, [0x10,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102d
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102e
            register_write(device_2905_1, reg_addr, [0x1d,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102f
            register_write(device_2905_1, reg_addr, [0xf0,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1030
            register_write(device_2905_1, reg_addr, [0x77,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1031
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1032
            register_write(device_2905_1, reg_addr, [0x10,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1033
            register_write(device_2905_1, reg_addr, [0x80,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1034
            register_write(device_2905_1, reg_addr, [0x80,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1035
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出彩条(va2905)
            #VPG配置(720p, 10fps, raw8)
            print("----------------10----------------")
            reg_addr=0x1001
            register_write(device_2905_1, reg_addr, [0x2a,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1004
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1005
            register_write(device_2905_1, reg_addr, [0x05,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1008
            #register_write(device_2905_1, reg_addr, [0xb6,])
            register_write(device_2905_1, reg_addr, [0xc4,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1009
            #register_write(device_2905_1, reg_addr, [0x03,])
            register_write(device_2905_1, reg_addr, [0x0b,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1010
            #register_write(device_2905_1, reg_addr, [0x6e,])
            register_write(device_2905_1, reg_addr, [0xca,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1011
            #register_write(device_2905_1, reg_addr, [0x0c,])
            register_write(device_2905_1, reg_addr, [0x1d,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1006
            register_write(device_2905_1, reg_addr, [0xd0,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1007
            register_write(device_2905_1, reg_addr, [0x02,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1014
            register_write(device_2905_1, reg_addr, [0x0f,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1015
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1012
            register_write(device_2905_1, reg_addr, [0xee,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1013
            register_write(device_2905_1, reg_addr, [0x02,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1041
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1042
            register_write(device_2905_1, reg_addr, [0x0a,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1063
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1064
            register_write(device_2905_1, reg_addr, [0x0a,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1000
            register_write(device_2905_1, reg_addr, [0x01,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')    
            
            
        finally:
            pass
            
def va2905_ch2_vpg_3():
        try:
            reg_addr=0x902b
            register_write_mod(device_2905_2, reg_addr, 0x3<<2, 0x2<<2)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4000
            register_write_mod(device_2905_2, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出yuv
            print("----------------10-------1---------")
            reg_addr=0x1016
            register_write(device_2905_2, reg_addr, [0xec,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1017
            register_write(device_2905_2, reg_addr, [0x80,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1018
            register_write(device_2905_2, reg_addr, [0x80,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1019
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101a
            register_write(device_2905_2, reg_addr, [0x4a,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101b
            register_write(device_2905_2, reg_addr, [0x61,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101c
            register_write(device_2905_2, reg_addr, [0x80,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101d
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101e
            register_write(device_2905_2, reg_addr, [0x7e,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x101f
            register_write(device_2905_2, reg_addr, [0x44,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1020
            register_write(device_2905_2, reg_addr, [0xcc,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1021
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1022
            register_write(device_2905_2, reg_addr, [0xde,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1023
            register_write(device_2905_2, reg_addr, [0x10,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1024
            register_write(device_2905_2, reg_addr, [0x89,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1025
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1026
            register_write(device_2905_2, reg_addr, [0xa4,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1027
            register_write(device_2905_2, reg_addr, [0x30,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1028
            register_write(device_2905_2, reg_addr, [0x19,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1029
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102a
            register_write(device_2905_2, reg_addr, [0xb1,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102b
            register_write(device_2905_2, reg_addr, [0x9f,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102c
            register_write(device_2905_2, reg_addr, [0x10,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102d
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102e
            register_write(device_2905_2, reg_addr, [0x1d,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x102f
            register_write(device_2905_2, reg_addr, [0xf0,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1030
            register_write(device_2905_2, reg_addr, [0x77,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1031
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1032
            register_write(device_2905_2, reg_addr, [0x10,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1033
            register_write(device_2905_2, reg_addr, [0x80,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1034
            register_write(device_2905_2, reg_addr, [0x80,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1035
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #VPG输出彩条(va2905)
            #VPG配置(720p, 10fps, YUV422 8-bit)
            print("----------------10----------------")
            reg_addr=0x1001
            register_write(device_2905_2, reg_addr, [0x1e,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1004
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1005
            register_write(device_2905_2, reg_addr, [0x05,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1008
            #register_write(device_2905_2, reg_addr, [0xb6,])
            register_write(device_2905_2, reg_addr, [0xc4,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1009
            #register_write(device_2905_2, reg_addr, [0x03,])
            register_write(device_2905_2, reg_addr, [0x0b,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1010
            #register_write(device_2905_2, reg_addr, [0x6e,])
            register_write(device_2905_2, reg_addr, [0xca,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1011
            #register_write(device_2905_2, reg_addr, [0x0c,])
            register_write(device_2905_2, reg_addr, [0x1d,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1006
            register_write(device_2905_2, reg_addr, [0xd0,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1007
            register_write(device_2905_2, reg_addr, [0x02,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1014
            register_write(device_2905_2, reg_addr, [0x0f,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1015
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1012
            register_write(device_2905_2, reg_addr, [0xee,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1013
            register_write(device_2905_2, reg_addr, [0x02,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1041
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1042
            register_write(device_2905_2, reg_addr, [0x0a,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1063
            register_write(device_2905_2, reg_addr, [0x00,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1064
            register_write(device_2905_2, reg_addr, [0x0a,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1000
            register_write(device_2905_2, reg_addr, [0x01,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')    
            
            
        finally:
            pass
            
def va2905_init_1():
        try:
            print("----------2905-----chan1-----------------")
            reg_addr=0x900c
            register_write_mod(device_2905_1, reg_addr, 0x7<<3, 0x6<<3)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
        
            #reg_addr=0x902f
            #register_write(device_2905_1, reg_addr, [0x4,])
            #reg = register_read(device_2905_1, reg_addr, 1)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]), end='\n')
            
            reg_addr=0x902c
            register_write_mod(device_2905_1, reg_addr, 0x1<<4, 0x1<<4)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x902c
            register_write_mod(device_2905_1, reg_addr, 0x1<<3, 0x1<<3)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x900c
            register_write_mod(device_2905_1, reg_addr, 0x7<<3, 0x1<<3)
            time.sleep(1)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0xffff
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            
            #配置SYS_CTRL视频通路选择寄存器
            print("----------2905------1----------------")
            reg_addr=0x902b
            register_write_mod(device_2905_1, reg_addr, 0x3<<2, 0x2<<2)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CSI2_HOST寄存器
            print("----------2905------2----------------")
            reg_addr=0x0184
            register_write_mod(device_2905_1, reg_addr, 0x1<<6, 0x1<<6)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018c
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018d
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018e
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018f
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0190
            register_write_mod(device_2905_1, reg_addr, 0x1f<<0, 0x8<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0194
            register_write_mod(device_2905_1, reg_addr, 0xf<<0, 0x7<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0254
            register_write_mod(device_2905_1, reg_addr, 0x3<<0, 0x2<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0184
            register_write_mod(device_2905_1, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CSI2_DEVICE寄存器
            print("----------2905------3----------------")
            reg_addr=0x3104
            register_write(device_2905_1, reg_addr, [0x1,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x3136
            register_write(device_2905_1, reg_addr, [0x1,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x3140
            register_write_mod(device_2905_1, reg_addr, 0x1<<2, 0x0<<2)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置PAL_CSI2寄存器
            print("----------2905------4----------------")
            reg_addr=0xe007
            register_write(device_2905_1, reg_addr, [0x1,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CDPHYRX寄存器
            print("----------2905------5----------------")
            va2905_cdphy_1_mpw()
            
            #配置SYS_CTRL寄存器打开CDPHYRX
            print("----------2905------6----------------")
            reg_addr=0x9030
            register_write_mod(device_2905_1, reg_addr, 0x3f<<0, 0x38<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0xffff
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9030
            register_write_mod(device_2905_1, reg_addr, 0x3f<<0, 0x39<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            time.sleep(1)
            print("-------0xb074-----------")
            reg_addr=0xb074
            #register_write_mod(device_2905_1, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xd004      #sensor reset
            register_write(device_2905_1, reg_addr, [0x20,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x3138      #add by 0410
            register_write(device_2905_1, reg_addr, [0x03,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
        finally:
            pass
 
def va2905_init_2():
        try:
            print("----------2905----chan2------------------")
            reg_addr=0x900c
            register_write_mod(device_2905_2, reg_addr, 0x7<<3, 0x6<<3)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
        
            #reg_addr=0x902f
            #register_write(device_2905_2, reg_addr, [0x4,])
            #reg = register_read(device_2905_2, reg_addr, 1)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]), end='\n')
            
            reg_addr=0x902c
            register_write_mod(device_2905_2, reg_addr, 0x1<<4, 0x1<<4)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x902c
            register_write_mod(device_2905_2, reg_addr, 0x1<<3, 0x1<<3)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x900c
            register_write_mod(device_2905_2, reg_addr, 0x7<<3, 0x1<<3)
            time.sleep(1)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0xffff
            register_write(device_2905_2, reg_addr, [0xff,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            
            #配置SYS_CTRL视频通路选择寄存器
            print("----------2905------1----------------")
            reg_addr=0x902b
            register_write_mod(device_2905_2, reg_addr, 0x3<<2, 0x2<<2)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CSI2_HOST寄存器
            print("----------2905------2----------------")
            reg_addr=0x0184
            register_write_mod(device_2905_2, reg_addr, 0x1<<6, 0x1<<6)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018c
            register_write(device_2905_2, reg_addr, [0xff,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018d
            register_write(device_2905_2, reg_addr, [0xff,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018e
            register_write(device_2905_2, reg_addr, [0xff,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018f
            register_write(device_2905_2, reg_addr, [0xff,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0190
            register_write_mod(device_2905_2, reg_addr, 0x1f<<0, 0x8<<0)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0194
            register_write_mod(device_2905_2, reg_addr, 0xf<<0, 0x7<<0)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0254
            register_write_mod(device_2905_2, reg_addr, 0x3<<0, 0x2<<0)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0184
            register_write_mod(device_2905_2, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CSI2_DEVICE寄存器
            print("----------2905------3----------------")
            reg_addr=0x3104
            register_write(device_2905_2, reg_addr, [0x1,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x3136
            register_write(device_2905_2, reg_addr, [0x1,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x3140
            register_write_mod(device_2905_2, reg_addr, 0x1<<2, 0x0<<2)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置PAL_CSI2寄存器
            print("----------2905------4----------------")
            reg_addr=0xe007
            register_write(device_2905_2, reg_addr, [0x1,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CDPHYRX寄存器
            print("----------2905------5----------------")
            va2905_cdphy_2()
            
            #配置SYS_CTRL寄存器打开CDPHYRX
            print("----------2905------6----------------")
            reg_addr=0x9030
            register_write_mod(device_2905_2, reg_addr, 0x3f<<0, 0x38<<0)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0xffff
            register_write(device_2905_2, reg_addr, [0xff,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9030
            register_write_mod(device_2905_2, reg_addr, 0x3f<<0, 0x39<<0)
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xd004      #sensor reset
            register_write(device_2905_2, reg_addr, [0x20,])
            reg = register_read(device_2905_2, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
        finally:
            pass
 
 
def va2905_vproc_comp_raw10():
        try:
            print("----------2905-----chan1------comp-----raw10------")

            reg_addr=0x4004
            register_write(device_2905_1, reg_addr, [0x5,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4005
            register_write(device_2905_1, reg_addr, [0xaa,])    #raw10
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #1296x972
            reg_addr=0x4009
            register_write(device_2905_1, reg_addr, [0x05,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4008
            register_write(device_2905_1, reg_addr, [0x10,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x400b
            register_write(device_2905_1, reg_addr, [0x03,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x400a
            register_write(device_2905_1, reg_addr, [0xcc,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #hactive*vactive*10/8/4 //设置为4倍压缩
            reg_addr=0x400f
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x400e
            register_write(device_2905_1, reg_addr, [0x06,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x400d
            register_write(device_2905_1, reg_addr, [0x01,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x400c
            register_write(device_2905_1, reg_addr, [0xbc,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
        finally:
            pass
 

def va2905_vproc_comp_yuv422():
        try:
            print("----------2905-----chan1------comp-----yuv422------")

            reg_addr=0x4004
            register_write(device_2905_1, reg_addr, [0x3,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4005
            register_write(device_2905_1, reg_addr, [0xa8,])    #yuv422
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #1280x720
            reg_addr=0x4009
            register_write(device_2905_1, reg_addr, [0x05,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4008
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x400b
            register_write(device_2905_1, reg_addr, [0x02,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x400a
            register_write(device_2905_1, reg_addr, [0xd0,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #hactive*vactive*10/8/4 //设置为4倍压缩
            reg_addr=0x400f
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x400e
            register_write(device_2905_1, reg_addr, [0x07,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x400d
            register_write(device_2905_1, reg_addr, [0x08,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x400c
            register_write(device_2905_1, reg_addr, [0x00,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
        finally:
            pass
            

def va2905_init_1_comp():
        try:
            print("----------2905-----chan1------comp-----------")
            reg_addr=0x900c
            register_write_mod(device_2905_1, reg_addr, 0x7<<3, 0x6<<3)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
        
            #reg_addr=0x902f
            #register_write(device_2905_1, reg_addr, [0x4,])
            #reg = register_read(device_2905_1, reg_addr, 1)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]), end='\n')
            
            reg_addr=0x902c
            register_write_mod(device_2905_1, reg_addr, 0x1<<4, 0x1<<4)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x902c
            register_write_mod(device_2905_1, reg_addr, 0x1<<3, 0x1<<3)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x900c
            register_write_mod(device_2905_1, reg_addr, 0x7<<3, 0x1<<3)
            time.sleep(1)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0xffff
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            
            #配置SYS_CTRL视频通路选择寄存器
            print("----------2905------1----------------")
            reg_addr=0x902b
            register_write_mod(device_2905_1, reg_addr, 0x3<<2, 0x1<<2)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CSI2_HOST寄存器
            print("----------2905------2----------------")
            reg_addr=0x0184
            register_write_mod(device_2905_1, reg_addr, 0x1<<6, 0x1<<6)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018c
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018d
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018e
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x018f
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0190
            register_write_mod(device_2905_1, reg_addr, 0x1f<<0, 0x8<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0194
            register_write_mod(device_2905_1, reg_addr, 0xf<<0, 0x7<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0254
            register_write_mod(device_2905_1, reg_addr, 0x3<<0, 0x2<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0184
            register_write_mod(device_2905_1, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置vproc寄存器
            print("----------2905------3-----comp-----------")
            va2905_vproc_comp_raw10()
            #va2905_vproc_comp_yuv422()
            
            #配置CSI2_DEVICE寄存器
            print("----------2905------3----------------")
            reg_addr=0x3104
            register_write(device_2905_1, reg_addr, [0x1,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x3136
            register_write(device_2905_1, reg_addr, [0x1,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x3140
            register_write_mod(device_2905_1, reg_addr, 0x1<<2, 0x0<<2)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置PAL_CSI2寄存器
            print("----------2905------4----------------")
            reg_addr=0xe007
            register_write(device_2905_1, reg_addr, [0x0,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CDPHYRX寄存器
            print("----------2905------5----------------")
            va2905_cdphy_1_mpw()
            
            #配置SYS_CTRL寄存器打开CDPHYRX
            print("----------2905------6----------------")
            reg_addr=0x9030
            register_write_mod(device_2905_1, reg_addr, 0x3f<<0, 0x38<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0xffff
            register_write(device_2905_1, reg_addr, [0xff,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x9030
            register_write_mod(device_2905_1, reg_addr, 0x3f<<0, 0x39<<0)
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xd004      #sensor reset
            register_write(device_2905_1, reg_addr, [0x20,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x3138      #add by 0410
            register_write(device_2905_1, reg_addr, [0x03,])
            reg = register_read(device_2905_1, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
        finally:
            pass 
       
def va2906_one_stream():
        try:
            print("----------one---stream-------------------") 
            #配置2905
            va2905_init_1()
        
            #2906配置SYS_CTRL时钟设置相关寄存器
            print("----------------1----------------")
            reg_addr=0x6c0c
            register_write_mod(device_2906, reg_addr, 0x7<<3, 0x6<<3)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c24
            register_write_mod(device_2906, reg_addr, 0x1<<1, 0x1<<1)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c24
            register_write_mod(device_2906, reg_addr, 0x1<<3, 0x1<<3)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c0c
            register_write_mod(device_2906, reg_addr, 0x7<<3, 0x1<<3)
            #reg = register_read(device_2906, reg_addr, 1)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]), end='\n')
            
            time.sleep(1)
            reg_addr=0x6c0c
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置L*_PAL_CSI寄存器
            print("----------------2----------------")
            reg_addr=0x0007
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置L*_CSIHOST寄存器
            print("----------------3----------------")
            reg_addr=0x4184
            register_write_mod(device_2906, reg_addr, 0x1<<6, 0x0<<6)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4190
            register_write_mod(device_2906, reg_addr, 0x1<<4, 0x0<<4)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4194
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x7<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4184
            register_write_mod(device_2906, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置LINEMEM寄存器
            print("----------------4----------------")
            reg_addr=0x5800
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5801
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置L*_CSIDEV寄存器
            print("----------------5----------------")
            reg_addr=0x5c3c
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x3<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5d04
            register_write(device_2906, reg_addr, [0x03,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5d2c
            register_write(device_2906, reg_addr, [0x02,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x5d30
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5d36
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CDPHYTX0寄存器
            print("----------------6----------------")
            va2906_cdphy_mpw()
            
            #配置SYS_CTRL寄存器打开CDPHYTX0
            print("----------------7----------------")
            reg_addr=0x6c26
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x8<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c26
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0xc<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')

            reg_addr=0x002c     #add by 0410
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x042c     #add by 0410
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            
        finally:
            pass
 
def va2906_one_stream_comp():
        try:
            print("----------one---stream------comp-------------") 
            #配置2905
            va2905_init_1_comp()
        
            #2906配置SYS_CTRL时钟设置相关寄存器
            print("----------------1----------------")
            reg_addr=0x6c0c
            register_write_mod(device_2906, reg_addr, 0x7<<3, 0x6<<3)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c24
            register_write_mod(device_2906, reg_addr, 0x1<<1, 0x1<<1)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c24
            register_write_mod(device_2906, reg_addr, 0x1<<3, 0x1<<3)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c0c
            register_write_mod(device_2906, reg_addr, 0x7<<3, 0x1<<3)
            #reg = register_read(device_2906, reg_addr, 1)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]), end='\n')
            
            time.sleep(1)
            reg_addr=0x6c0c
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置SYS_CTRL视频通路选择寄存器
            print("----------------2----------------")
            reg_addr=0x6c20
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置L*_PAL_CSI寄存器
            print("----------------2----------------")
            reg_addr=0x0007
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置L*_CSIHOST寄存器
            print("----------------3----------------")
            reg_addr=0x4184
            register_write_mod(device_2906, reg_addr, 0x1<<6, 0x0<<6)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4190
            register_write_mod(device_2906, reg_addr, 0x1<<4, 0x0<<4)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4194
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x7<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4184
            register_write_mod(device_2906, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置vproc寄存器
            print("----------------4-----comp-----------")
            reg_addr=0x5408
            register_write(device_2906, reg_addr, [0x01,])  #使能第1路link解压功能
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置LINEMEM寄存器
            print("----------------4----------------")
            reg_addr=0x5800
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5801
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置L*_CSIDEV寄存器
            print("----------------5----------------")
            reg_addr=0x5c3c
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x0<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5d04
            register_write(device_2906, reg_addr, [0x03,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5d2c
            register_write(device_2906, reg_addr, [0x02,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x5d30
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5d36
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CDPHYTX0寄存器
            print("----------------6----------------")
            va2906_cdphy_mpw()
            
            #配置SYS_CTRL寄存器打开CDPHYTX0
            print("----------------7----------------")
            reg_addr=0x6c26
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x8<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c26
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0xc<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')

            reg_addr=0x002c     #add by 0410
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x042c     #add by 0410
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
        finally:
            pass
            
            
def va2906_two_stream():
        try:
            print("----------two---stream-------------------") 
            #配置2905
            va2905_init_1()
            va2905_init_2()
            
            #2906配置SYS_CTRL时钟设置相关寄存器
            print("----------------1----------------")
            reg_addr=0x6c0c
            register_write_mod(device_2906, reg_addr, 0x7<<3, 0x6<<3)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c24
            register_write_mod(device_2906, reg_addr, 0x1<<1, 0x1<<1)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c24
            register_write_mod(device_2906, reg_addr, 0x1<<3, 0x1<<3)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c0c
            register_write_mod(device_2906, reg_addr, 0x7<<3, 0x1<<3)
            #reg = register_read(device_2906, reg_addr, 1)
            #print(hex(reg_addr), end="\t")
            #print (hex(reg[0]), end='\n')
            
            time.sleep(1)
            reg_addr=0x6c0c
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置L*_PAL_CSI寄存器
            print("----------------2----------------")
            reg_addr=0x0007
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x0407     #add by 0417 2905_2
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置L*_CSIHOST寄存器
            print("----------------3----------------")
            reg_addr=0x4184
            register_write_mod(device_2906, reg_addr, 0x1<<6, 0x0<<6)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4190
            register_write_mod(device_2906, reg_addr, 0x1<<4, 0x0<<4)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4194
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x7<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4184
            register_write_mod(device_2906, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            ## for 2905-2
            reg_addr=0x4584
            register_write_mod(device_2906, reg_addr, 0x1<<6, 0x0<<6)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4590
            register_write_mod(device_2906, reg_addr, 0x1<<4, 0x0<<4)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4594
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x7<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x4584
            register_write_mod(device_2906, reg_addr, 0x1<<0, 0x1<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置LINEMEM寄存器
            print("----------------4----------------")
            reg_addr=0x5800
            register_write(device_2906, reg_addr, [0x03,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5801
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置L*_CSIDEV寄存器
            print("----------------5----------------")
            reg_addr=0x5c3c
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x0<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5d04
            register_write(device_2906, reg_addr, [0x03,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5d2c
            register_write(device_2906, reg_addr, [0x02,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #add by 0420
            reg_addr=0x5d30
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x5d36
            register_write(device_2906, reg_addr, [0x00,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            #配置CDPHYTX0寄存器
            print("----------------6----------------")
            va2906_cdphy_mpw()
            
            #配置SYS_CTRL寄存器打开CDPHYTX0
            print("----------------7----------------")
            reg_addr=0x6c26
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0x8<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x6c26
            register_write_mod(device_2906, reg_addr, 0xf<<0, 0xc<<0)
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')

            reg_addr=0x002c     #add by 0410
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x042c     #add by 0410
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            
            
        finally:
            pass
            
     
            
            
if __name__ == '__main__':
        
     
        debug=1
        init=1
        
        #va2905_cdphy_1_mpw()
        #va2906_cdphy_mpw()
        
        if init > 0:
            #配置2906 的ICAM/ICAS PAL
            reg_addr=0x1006
            register_write(device_2906, reg_addr, [0x58,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1007
            register_write(device_2906, reg_addr, [0x50,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1018
            register_write(device_2906, reg_addr, [0x5,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1026
            register_write(device_2906, reg_addr, [0x6,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1008
            register_write(device_2906, reg_addr, [0x28,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1010
            register_write(device_2906, reg_addr, [0x18,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1009
            register_write(device_2906, reg_addr, [0x36,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1011
            register_write(device_2906, reg_addr, [0x36,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x100a
            register_write(device_2906, reg_addr, [0x3c,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1012
            register_write(device_2906, reg_addr, [0x3c,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1021
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1406
            register_write(device_2906, reg_addr, [0x58,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1407
            register_write(device_2906, reg_addr, [0x51,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1418
            register_write(device_2906, reg_addr, [0x5,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1426
            register_write(device_2906, reg_addr, [0x6,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1408
            register_write(device_2906, reg_addr, [0x28,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1410
            register_write(device_2906, reg_addr, [0x19,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1409
            register_write(device_2906, reg_addr, [0x36,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1411
            register_write(device_2906, reg_addr, [0x37,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x140a
            register_write(device_2906, reg_addr, [0x3c,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1412
            register_write(device_2906, reg_addr, [0x3d,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x1421
            register_write(device_2906, reg_addr, [0x01,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x2046
            register_write(device_2906, reg_addr, [0x5,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0x2446
            register_write(device_2906, reg_addr, [0x5,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
                
            #gpio reset
            reg_addr=0xc004
            register_write(device_2906, reg_addr, [0x20,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
            
            reg_addr=0xc005
            register_write(device_2906, reg_addr, [0x20,])
            reg = register_read(device_2906, reg_addr, 1)
            print(hex(reg_addr), end="\t")
            print (hex(reg[0]), end='\n')
        
        else:
            pass
        
        va2906_one_stream()
        #va2906_one_stream_comp()
        #va2906_two_stream()
        
        if debug > 0:
            
            
            #配置sensor打开视频流
            print("----------------8----------------")
            
            #VPG输出彩条(va2906)
            #Csi video processor unit 中选择数据输入源为VPG
            print("----------------2906-vpg---------------")
            #va2906_vpg()   #720px30    yuv422
            #va2906_vpg_2() #640px30    yuv422
            #va2906_vpg_3()  #720px10   yuv422
            
            
            #VPG输出彩条(va2905)
            print("----------------2905--vpg--------------")
            #va2905_vpg_1() #720px30 yuv422
            va2905_vpg_3() #720px10 yuv422
            #va2905_vpg_4() #720px10 raw8
            #va2905_ch2_vpg_3()  #720px10   yuv422
        else:
            pass
        
        bus.close()


