Set wbemLocator = CreateObject("WbemScripting.SWbemLocator")
Set wbemServices = wbemLocator.ConnectServer(, "root\wmi")
wbemServices.Security_.ImpersonationLevel=3 ' i.e., impersonal
Set wbemObjectSet = wbemServices.InstancesOf("WMI42")

For Each wbemObject in wbemObjectSet
  WScript.Echo "Name:      " & wbemObject.Path_ & vbCrlf &_
               "TheAnswer: " & wbemObject.TheAnswer
  wbemObject.TheAnswer = wbemObject.TheAnswer + 1
  wbemObject.Put_()
Next
