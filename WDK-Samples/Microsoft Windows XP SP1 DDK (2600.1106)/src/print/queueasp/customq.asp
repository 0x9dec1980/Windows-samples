<%
    Option Explicit
    Session("PrintSupportEmail") = "Helpdesk"
    Session("PrintSupportPhone") = "555-1717"
%>
<%
    Response.Expires = 0
    Dim strPrinter, strAction, strComputer, strJobid, objQueue, objJobs, strSelected, objJob, iRes, bDHTML
    Dim strIPrintDebug
    dim intLoop

    Session("view") = "q"
    ' Display contact information for users
    Response.write "<strong>Need support? E-mail <em><a href=""mailto:" & session("PrintSupportEmail") & """>" & session("PrintSupportEmail") & "</a></em> or call <em>" & session("PrintSupportPhone") & "</em></strong>" & vbCrLf
    if session("IPrintDebug")<>"" then
        DumpIPrintVars
    end if

    dim OleCvt

    Const PROGID_CONVERTER      = "OlePrn.OleCvt"
    Set OleCvt = Server.CreateObject (PROGID_CONVERTER)

    strPrinter = Request.QueryString("eprinter")
    strAction = Request.QueryString("action")
    strJobid = Request.QueryString("jobid")

    strPrinter = OleCvt.DecodeUnicodeName (strPrinter)

    strComputer = Session("MS_Computer")
    bDHTML = Session("MS_DHTMLEnabled")

    On Error Resume Next
    Err.Clear

    Set objQueue = GetObject("WinNT://" & strComputer & "/" & strPrinter & ",PrintQueue")
    If Err Then ErrorHandler ("Open Queue")
    Set objJobs = objQueue.PrintJobs
    If Err Then ErrorHandler ("Get Jobs")
    If strAction <> "" Then
        DoActions objJobs, strAction, strJobid
        If Err.Number <> 0 Then ErrorHandler ("Do Actions")
    End If

    If strSelected = "" Then
        For Each objJob In objJobs
            strSelected = objJob.Name
            Exit For
        Next
    End If

Sub DoActions(objJobs, strAction, strJobid)
    Dim bFound, objJob
    If strAction = "" Or strJobid = "" Then Exit Sub
    bFound = False
    For Each objJob In objJobs
        If objJob.Name = strJobid Then
            bFound = True
            Exit For
        End If
    Next

' can't find the job - assume it disappeared. Just exit and say we did it

    If bFound = False Then Exit Sub

    On Error Resume Next
    Err.Clear
    Select Case strAction
        Case "cancel"
            objJobs.Remove CStr(strJobid)
        Case "pause"
            objJob.pause
        Case "resume"
            objJob.resume
    End Select

End Sub

Function strJobStatus(iStatus)
    Dim L_JobStatus(11)
    Dim bit, i
    Dim strTemp, bFirst
    Const L_Seperator = " - "

    L_JobStatus(0) = "Paused"
    L_JobStatus(1) = "Error"
    L_JobStatus(2) = "Deleting"
    L_JobStatus(3) = "Spooling"
    L_JobStatus(4) = "Printing"
    L_JobStatus(5) = "Offline"
    L_JobStatus(6) = "Out of Paper"
    L_JobStatus(7) = "Printed"
    L_JobStatus(8) = "Deleted"
    L_JobStatus(9) = "Blocked"
    L_JobStatus(10) = "User Intervention Required"
    L_JobStatus(11) = "Restarting"

    bit = 1
    i = 0

    bFirst = True
    strTemp = ""

    For i = 0 To 11
        If iStatus And bit Then
            If Not bFirst Then
                strTemp = strTemp + L_Seperator
            End If
                strTemp = strTemp + L_JobStatus(i)
                bFirst = False
        End If
        bit = bit * 2
    Next
    If strTemp = "" Then strTemp = "&nbsp;"

    strJobStatus = strTemp
End Function

%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN"><html>
<head>
<meta http-equiv="Refresh" content="30; URL=Page1.ASP">
<title>Custom Queue</title>
</head>
<body bgcolor="#FFFFFF" text="#000000" topmargin="0" leftmargin="0" link="#000000" vlink="#000000" alink="#000000">
<%
   'Set contact names
   session("PrintSupportEmail")="printsupport"
   session("PrintSupportPhone")="555-1717"

    Dim objPrinter
    Set objPrinter = Server.CreateObject (PROGID_HELPER)
    objPrinter.open strPrinter
    call addJobInformation (objPrinter)
    objPrinter.Close
    DumpSessionVars
%>

<br>

<script language="javascript">
function setJobId (id)
{
    document.forms[1].elements[0].value = id;
    return true;
}

</script>

<!-- The following line is for the compatibility with IE3-->
<p><br></p>

<form >
  <input type="hidden" name="selectedid" value="<%=strJobid%>">
</form>

<form>
<%if bDHTML then %>
  <div
  ONCLICK="colorSelector()"><script LANGUAGE="JavaScript">
        var oldTr = 0;
    function colorSelector()
    {
        var jlist = document.all.JOBLIST.rows;

        if (!jlist[0].contains (event.srcElement))
        {
            if (oldTr != null)
            {
                oldTr.bgColor = "<%=UnselectedColor%>";
            }

            for (i = 1; i < jlist.length; i++)
            {
               if (jlist[i].contains (event.srcElement))
                {
                    oldTr = jlist[i];
                    oldTr.bgColor = "<%=SelectedColor%>";
                    document.forms[0].elements[oldTr.id - <%=iStart%> + 1].checked = true;
                    setJobId (document.forms[0].elements[oldTr.id - <%=iStart%> + 1].value);
                }
            }
        }
    }


</script>
<% end if%>
<%
Const L_TableHeader1 = "<td width=""30%"" bgcolor=""#000000"" nowrap><font color=""#FFFFFF""><strong>Document</strong></font></td>"
Const L_TableHeader2 = "<td width=""10%"" bgcolor=""#000000"" nowrap><font color=""#FFFFFF""><strong>Status</strong></font></td>"
Const L_TableHeader3 = "<td width=""10%"" bgcolor=""#000000"" nowrap><font color=""#FFFFFF""><strong>Owner</strong></font></td>"
Const L_TableHeader4 = "<td width=""10%"" bgcolor=""#000000"" nowrap><font color=""#FFFFFF""><strong>Pages</strong></font></td>"
Const L_TableHeader5 = "<td width=""10%"" bgcolor=""#000000"" nowrap><font color=""#FFFFFF""><strong>Size</strong></font></td>"
Const L_TableHeader6 = "<td width=""20%"" bgcolor=""#000000"" nowrap><font color=""#FFFFFF""><strong>Submitted</strong></font></td>"
%>
<table id="JOBLIST"
  border="0" cellpadding="2" cellspacing="0" width="100%">
    <tr>
    <%= L_TableHeader1 & L_TableHeader2 & L_TableHeader3 & L_TableHeader4 & L_TableHeader5 & L_TableHeader6%>
    </tr>
<%
        dim strColor, index, i
        i = 1
        index = -1

        for each objJob in objJobs
                if objJob.name = strJobid then
                        index = i
                end if
    %>
    <tr bgcolor="#ffffff" id="<%=i-1%>">
      <td width="30%" nowrap>&nbsp;<input type="radio" name="jobid" value="<% =objJob.name%>"
      <% if objJob.name = strJobid then%> checked <% end if%>
      onclick="return setJobId(<% =objJob.name%>);"><% = objjob.Description %></td>
      <td width="
        10%"><% =strJobStatus(objJob.status) %>
</td>
      <td width="10%" nowrap><% = objJob.user %>
</td>
      <td width="10%" nowrap>&nbsp; <% if objJob.totalpages > 0 then %> <%= objJob.totalpages %> <% end if %> </td>
      <td width="10%" nowrap>&nbsp;<%
        if objJob.size > 0 then%> <%=strFormatJobSize (objJob.size)%> <% end if %> </td>
      <td width="20%" nowrap><% = formatdatetime(objJob.timesubmitted, 3) & " " & formatdatetime(objJob.timesubmitted, 2)%>
</td>
    </tr>
<% i = i + 1
next %>
  </table>
</form>
<%if bDHTML then%>
  </div>
<% end if %>
<%if index > 0 then%>
<script LANGUAGE="JavaScript">

        window.onerror=windowError;
        function windowError ()
        {       return true; }

        if (document.all) {
        oldTr = document.all.JOBLIST.rows[<%=index%>];
        oldTr.bgColor = "#c0c0c0";
    }
</script>
<%end if%>
</body>
</html>


