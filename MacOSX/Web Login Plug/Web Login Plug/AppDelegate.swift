//
//  AppDelegate.swift
//  Web Login Plug
//

import Cocoa
import IOKit

@main
class AppDelegate: NSObject, NSApplicationDelegate {

    var MacAddress = String.init("")
    let HTTPServer = HttpServer()
    let statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
    //        let statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.squareLength)
    
    func applicationDidFinishLaunching(_ aNotification: Notification) {
        statusItem.toolTip = "Web Login Plug"
        if let button = statusItem.button {
            button.image = NSImage(named: "statusIcon")
        }
        let statusMenu = NSMenu()
        statusMenu.addItem(withTitle: "Copy Mac Address",action: #selector(Copy),keyEquivalent: "")
        statusMenu.addItem(withTitle: "------ v1.0.2 ------",action: nil,keyEquivalent: "")
        statusMenu.addItem(withTitle: "Hide the icon",action: #selector(HideStatusIcon),keyEquivalent: "")
        statusMenu.addItem(withTitle: "Quit", action: #selector(ExitApp),keyEquivalent: "")
        statusItem.menu = statusMenu
        
        //Start the http service
        startHttpServer()
        
        //A method of obtaining mac address
//        if let intfIterator = FindEthernetInterfaces() {
//            if let macAddress = GetMACAddress(intfIterator) {
//                let macAddressAsString = macAddress.map( { String(format:"%02x", $0) } )
//                    .joined(separator: ":")
//                print(macAddressAsString)
//            }
//
//            IOObjectRelease(intfIterator)
//        }
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        NSApplication.shared.terminate(self)
        HTTPServer.stop()
    }

    func applicationSupportsSecureRestorableState(_ app: NSApplication) -> Bool {
        return true
    }

    func startHttpServer(){
        do {
            
            MacAddress = getDeviceUUid() ?? ""
            
            var requestData:Dictionary<String,Any> = [:]
            if(MacAddress == "00:00:00:00:00:00"){
                requestData["code"] = -1
                requestData["data"] = ""
                requestData["message"] = "Fail"
            }else{
                requestData["code"] = 0
                requestData["data"] = MacAddress
                requestData["message"] = "Success"
            }
            HTTPServer["/v1/info"] = { _ in
                return .ok(.json(requestData),["Access-Control-Allow-Origin":"*","Access-Control-Allow-Credentials":"true"])
            }
            try HTTPServer.start(32156, forceIPv4: true)

            print("Server has started ( port = \(try HTTPServer.port()) ). Try to connect now...")

        } catch {
            print("Server start error: \(error)")
        }
    }
    
    @objc func Copy(){
        MacAddress = getDeviceUUid() ?? ""
        if MacAddress != "" {
            //copy to clipboard
            let pboard = NSPasteboard.general
            pboard.declareTypes([.string], owner: nil)
            pboard.setString(MacAddress, forType: .string)
            
            //Pop-up reminder
            let myPopup: NSAlert = NSAlert()
            myPopup.messageText = "Copy Success"
            myPopup.informativeText = "Your Mac Address:\n\(MacAddress)"
            myPopup.alertStyle = NSAlert.Style.warning
            myPopup.addButton(withTitle: "Colse")
            myPopup.runModal()
        }
    }
    
    @objc func HideStatusIcon() {
        print("HideStatusIcon")
        statusItem.toolTip = ""
        if let button = statusItem.button {
            button.image = NSImage(named: "AccentColor")
        }
    }

    @objc func ExitApp() {
        print("ExitApp")
        let myPopup: NSAlert = NSAlert()
        myPopup.messageText = "Friendly Tips"
        myPopup.informativeText = "You can't login in after 'Quit'. Are you sure you want to 'Quit'?"
        myPopup.alertStyle = NSAlert.Style.warning
        myPopup.addButton(withTitle: "Yes")
        myPopup.addButton(withTitle: "No")
        switch(myPopup.runModal()){
            case NSApplication.ModalResponse.alertFirstButtonReturn:
                //Turn off the http service
                HTTPServer.stop()
                //Exit application
                NSApplication.shared.terminate(self)
                break
            case NSApplication.ModalResponse.alertSecondButtonReturn:
                break
            default:
                break
        }
    }
    
    func getDeviceUUid() -> String? {
        let platformExpert = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice"))
        defer { IOObjectRelease(platformExpert) }
        
        guard platformExpert != 0 else {
            //Get the network card mac address en0: the first network card (viewed by ifconfig)
            print("Get the network card mac address en0: the first network card (viewed by ifconfig)")
            if let macAddr = MACAddressForBSD(bsd: "en0"){
                return macAddr
            }
            return nil
        }
        return IORegistryEntryCreateCFProperty(platformExpert, kIOPlatformUUIDKey as CFString, kCFAllocatorDefault, 0).takeRetainedValue() as? String
    }
    
    // releasing the iterator after the caller is done with it.
//    func FindEthernetInterfaces() -> io_iterator_t? {
//
//        let matchingDict = IOServiceMatching("IOEthernetInterface") as NSMutableDictionary
//            matchingDict["IOPropertyMatch"] = [ "IOPrimaryInterface" : true]
//        // Note that another option here would be:
//        // matchingDict = IOBSDMatching("en0");
//        // but en0: isn't necessarily the primary interface, especially on systems with multiple Ethernet ports.
//
//        var matchingServices : io_iterator_t = 0
//        if IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &matchingServices) != KERN_SUCCESS {
//            return nil
//        }
//
//        return matchingServices
//    }
//
//    // Given an iterator across a set of Ethernet interfaces, return the MAC address of the last one.
//    // If no interfaces are found the MAC address is set to an empty string.
//    // In this sample the iterator should contain just the primary interface.
//    func GetMACAddress(_ intfIterator : io_iterator_t) -> [UInt8]? {
//
//        var macAddress : [UInt8]?
//
//        var intfService = IOIteratorNext(intfIterator)
//        while intfService != 0 {
//
//            var controllerService : io_object_t = 0
//            if IORegistryEntryGetParentEntry(intfService, "IOService", &controllerService) == KERN_SUCCESS {
//
//                let dataUM = IORegistryEntryCreateCFProperty(controllerService, "IOMACAddress" as CFString, kCFAllocatorDefault, 0)
//                if let data = dataUM?.takeRetainedValue() as? NSData {
//                    macAddress = [0, 0, 0, 0, 0, 0]
//                    data.getBytes(&macAddress!, length: macAddress!.count)
//                }
//                IOObjectRelease(controllerService)
//            }
//
//            IOObjectRelease(intfService)
//            intfService = IOIteratorNext(intfIterator)
//        }
//
//        return macAddress
//    }
    
    func MACAddressForBSD(bsd : String) -> String?
    {
        let MAC_ADDRESS_LENGTH = 6
        let separator = ":"

        var length : size_t = 0
        var buffer : [CChar]

        let bsdIndex = Int32(if_nametoindex(bsd))
        if bsdIndex == 0 {
            print("Error: could not find index for bsd name \(bsd)")
            return nil
        }
        let bsdData = Data(bsd.utf8)
        var managementInfoBase = [CTL_NET, AF_ROUTE, 0, AF_LINK, NET_RT_IFLIST, bsdIndex]

        if sysctl(&managementInfoBase, 6, nil, &length, nil, 0) < 0 {
            print("Error: could not determine length of info data structure");
            return nil;
        }

        buffer = [CChar](unsafeUninitializedCapacity: length, initializingWith: {buffer, initializedCount in
            for x in 0..<length { buffer[x] = 0 }
            initializedCount = length
        })

        if sysctl(&managementInfoBase, 6, &buffer, &length, nil, 0) < 0 {
            print("Error: could not read info data structure");
            return nil;
        }

        let infoData = Data(bytes: buffer, count: length)
        let indexAfterMsghdr = MemoryLayout<if_msghdr>.stride + 1
        let rangeOfToken = infoData[indexAfterMsghdr...].range(of: bsdData)!
        let lower = rangeOfToken.upperBound
        let upper = lower + MAC_ADDRESS_LENGTH
        let macAddressData = infoData[lower..<upper]
        let addressBytes = macAddressData.map{ String(format:"%02x", $0) }
        return addressBytes.joined(separator: separator)
    }
}

