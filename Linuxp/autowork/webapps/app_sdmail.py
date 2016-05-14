#coding: utf-8
import smtplib
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart

HOST = "mail.beike.com"
TO = "to@beike.com"
FROM = "from@beike.com"
SUBJECT = u"官网业务服务质量周报"

def addimg(src, imgid):
    fp = open(src, 'rb')
    msgImage = MIMEImage(fp.read())
    fp.close()
    msgImage.add_header('Content-ID', imgid)
    return msgImage

msg = MIMEMultipart('related')
msgtext = MIMEText("<font color=red>官网业务周平均延时图表:<br><img src=\"cid:weekly\" border=\"1\"><br>详细内容见附件。</font>","html","utf-8")
#msg = MIMEText("<font color=red>官网业务周平均延时图表:<br><img src=\"cid:weekly\" border=\"1\"><br>详细内容见附件。</font>","html","utf-8")
msgxlsx = MIMEText(open("app_sdmail.xlsx", "rb").read(), "base64", "utf-8")
msgxlsx["Content-Type"] = "application/octet-stream"
msg.attach(msgtext)
msg.attach(addimg("app_sdmail.png", "weekly"))
msg.attach(msgxlsx)

msg['Subject'] = SUBJECT
msg['From'] = FROM
msg['To'] = TO
try:
    server = smtplib.SMTP()
    server.connect(HOST, "25")
    server.starttls()
    server.login("test@beike.com", "123456")
    server.sendmail(FROM, TO, msg.as_string())
    server.quit()
    print "邮件发送成功！"
except Exception, e:  
    print "失败："+str(e) 
