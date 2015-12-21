/*
Navicat SQL Server Data Transfer

Source Server         : local
Source Server Version : 105000
Source Host           : localhost:1433
Source Database       : BreezeNet
Source Schema         : dbo

Target Server Type    : SQL Server
Target Server Version : 105000
File Encoding         : 65001

Date: 2015-12-21 22:57:09
*/


-- ----------------------------
-- Table structure for DBG_SerialChecks
-- ----------------------------
DROP TABLE [dbo].[DBG_SerialChecks]
GO
CREATE TABLE [dbo].[DBG_SerialChecks] (
[RecordID] int NOT NULL IDENTITY(1,1) ,
[SerialCheck] int NOT NULL DEFAULT ((0)) ,
[SerialGetInfo] int NOT NULL DEFAULT ((0)) 
)


GO

-- ----------------------------
-- Records of DBG_SerialChecks
-- ----------------------------
SET IDENTITY_INSERT [dbo].[DBG_SerialChecks] ON
GO
SET IDENTITY_INSERT [dbo].[DBG_SerialChecks] OFF
GO

-- ----------------------------
-- Table structure for WarIncConversionUsers
-- ----------------------------
DROP TABLE [dbo].[WarIncConversionUsers]
GO
CREATE TABLE [dbo].[WarIncConversionUsers] (
[WarIncCustomerID] int NOT NULL ,
[ConversionDate] datetime NULL ,
[SerialType] int NULL ,
[SerialKey] varchar(128) NULL 
)


GO

-- ----------------------------
-- Records of WarIncConversionUsers
-- ----------------------------

-- ----------------------------
-- Table structure for WarZDeletedOrders
-- ----------------------------
DROP TABLE [dbo].[WarZDeletedOrders]
GO
CREATE TABLE [dbo].[WarZDeletedOrders] (
[RecordID] int NOT NULL IDENTITY(1,1) ,
[Method] varchar(16) NULL ,
[Amount] float(53) NULL ,
[OrderID] varchar(128) NULL ,
[ApprovalCode] varchar(128) NULL ,
[email] varchar(256) NULL ,
[AccountType] int NULL ,
[Serial1] varchar(128) NULL ,
[Serial2] varchar(128) NULL ,
[Serial3] varchar(128) NULL ,
[Serial4] varchar(128) NULL 
)


GO

-- ----------------------------
-- Records of WarZDeletedOrders
-- ----------------------------
SET IDENTITY_INSERT [dbo].[WarZDeletedOrders] ON
GO
SET IDENTITY_INSERT [dbo].[WarZDeletedOrders] OFF
GO

-- ----------------------------
-- Table structure for WarZNewsletterSubscribers
-- ----------------------------
DROP TABLE [dbo].[WarZNewsletterSubscribers]
GO
CREATE TABLE [dbo].[WarZNewsletterSubscribers] (
[RecordID] int NOT NULL IDENTITY(1,1) ,
[email] varchar(128) NOT NULL ,
[SubscribeDate] datetime NULL ,
[OutOut] int NOT NULL DEFAULT ((0)) 
)


GO

-- ----------------------------
-- Records of WarZNewsletterSubscribers
-- ----------------------------
SET IDENTITY_INSERT [dbo].[WarZNewsletterSubscribers] ON
GO
SET IDENTITY_INSERT [dbo].[WarZNewsletterSubscribers] OFF
GO

-- ----------------------------
-- Table structure for WarZPreorders
-- ----------------------------
DROP TABLE [dbo].[WarZPreorders]
GO
CREATE TABLE [dbo].[WarZPreorders] (
[RecordID] int NOT NULL IDENTITY(1,1) ,
[OrderDate] datetime NULL ,
[ReferrerName] varchar(64) NULL ,
[Method] varchar(16) NULL ,
[Amount] float(53) NULL ,
[OrderID] varchar(128) NULL ,
[ApprovalCode] varchar(128) NULL ,
[email] varchar(256) NULL ,
[AccountType] int NULL ,
[Serial1] varchar(128) NULL ,
[Serial2] varchar(128) NULL ,
[Serial3] varchar(128) NULL ,
[Serial4] varchar(128) NULL ,
[ClientIP] varchar(32) NULL ,
[GeoCode] varchar(4) NULL 
)


GO

-- ----------------------------
-- Records of WarZPreorders
-- ----------------------------
SET IDENTITY_INSERT [dbo].[WarZPreorders] ON
GO
SET IDENTITY_INSERT [dbo].[WarZPreorders] OFF
GO

-- ----------------------------
-- Table structure for WarZSerials
-- ----------------------------
DROP TABLE [dbo].[WarZSerials]
GO
CREATE TABLE [dbo].[WarZSerials] (
[SerialID] int NOT NULL IDENTITY(1,1) ,
[SerialKey] varchar(128) NOT NULL ,
[SerialType] int NOT NULL DEFAULT ((0)) ,
[IsUsed] int NOT NULL DEFAULT ((0)) ,
[email] varchar(128) NULL ,
[CustomerID] int NOT NULL DEFAULT ((0)) ,
[ForumUser] int NOT NULL DEFAULT ((0)) ,
[IsBlocked] int NOT NULL DEFAULT ((0)) 
)


GO

-- ----------------------------
-- Records of WarZSerials
-- ----------------------------
SET IDENTITY_INSERT [dbo].[WarZSerials] ON
GO
SET IDENTITY_INSERT [dbo].[WarZSerials] OFF
GO

-- ----------------------------
-- Table structure for XsollaCancels
-- ----------------------------
DROP TABLE [dbo].[XsollaCancels]
GO
CREATE TABLE [dbo].[XsollaCancels] (
[x_id] int NOT NULL ,
[x_v1] varchar(255) NULL ,
[x_datetime] varchar(32) NULL ,
[xOrderDate] datetime NULL ,
[xCancelDate] datetime NULL 
)


GO

-- ----------------------------
-- Records of XsollaCancels
-- ----------------------------

-- ----------------------------
-- Table structure for XsollaOrders
-- ----------------------------
DROP TABLE [dbo].[XsollaOrders]
GO
CREATE TABLE [dbo].[XsollaOrders] (
[xOrderID] int NOT NULL IDENTITY(1,1) ,
[xClientIP] varchar(32) NULL ,
[xOrderDate] datetime NULL ,
[x_id] varchar(128) NOT NULL ,
[x_v1] varchar(255) NULL ,
[x_v2] varchar(200) NULL ,
[x_v3] varchar(100) NULL ,
[x_amount] float(53) NULL ,
[x_currency] varchar(32) NULL ,
[x_datetime] varchar(32) NULL 
)


GO

-- ----------------------------
-- Records of XsollaOrders
-- ----------------------------
SET IDENTITY_INSERT [dbo].[XsollaOrders] ON
GO
SET IDENTITY_INSERT [dbo].[XsollaOrders] OFF
GO

-- ----------------------------
-- Procedure structure for BN_WarInc_GetConversionSerial
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarInc_GetConversionSerial]
GO
CREATE PROCEDURE [dbo].[BN_WarInc_GetConversionSerial]
	@in_WarIncCustomerID int,
	@in_ConversionStatus int
AS
BEGIN
	SET NOCOUNT ON;
	
	-- check if we already have that customer
	declare @SerialKey varchar(128) = ''
	select @SerialKey=SerialKey from WarIncConversionUsers where WarIncCustomerID=@in_WarIncCustomerID
	if(@@ROWCOUNT > 0) begin
		select @SerialKey as 'SerialKey'
		return
	end
	
	declare @SerialType int = 3
	
	if(@in_ConversionStatus = 1) begin
		set @SerialType = 7 -- 7 days guest key
	end
	else if (@in_ConversionStatus = 2) begin
		set @SerialType = 4 -- 15$ package 
	end
	else begin
		select 'invalid conversion status' as 'Serial Key'
		return
	end

	-- get serial
	exec BN_WarZ_GetSerial 'warincconversion@thewarz.com', @SerialType, @SerialKey out
	
	-- store it
	insert into WarIncConversionUsers (
		WarIncCustomerID,
		ConversionDate,
		SerialType,
		SerialKey
	) values (
		@in_WarIncCustomerID,
		GETDATE(),
		@SerialType,
		@SerialKey
	)
		
	select @SerialKey as 'SerialKey'
	return
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_CheckSerial
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_CheckSerial]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_CheckSerial]
	@in_SerialKey varchar(128)
AS
BEGIN
	SET NOCOUNT ON;
	
	-- temp function used for checking if serial exists

	--select * from WarZSerials where SerialKey=@in_SerialKey	
	--return
  select 0 as ResultCode
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_GetSerial
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_GetSerial]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_GetSerial]
	@in_email varchar(128),
	@in_SerialType int,
	@out_SerialKey varchar(128) out
AS
BEGIN
	SET NOCOUNT ON;
	
	declare @SerialKey varchar(128) = 'NO-FREE-SERIALS'
	update top(1) WarZSerials set 
		@SerialKey=SerialKey, 
		IsUsed=1,
		SerialType=@in_SerialType,
		email=@in_email
	where IsUsed=0
	
	set @out_SerialKey = @SerialKey
	return
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_NewsletterGetCount
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_NewsletterGetCount]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_NewsletterGetCount]
AS
BEGIN
	SET NOCOUNT ON;
	
	--select 0 as ResultCode
	select count(*) as 'Count' from WarZNewsletterSubscribers
	
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_NewsletterSubscribe
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_NewsletterSubscribe]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_NewsletterSubscribe]
	@in_email varchar(128)
AS
BEGIN
	SET NOCOUNT ON;
	
	if exists (select email from WarZNewsletterSubscribers where email=@in_email)
	begin
		select 1 as ResultCode, 'already subscribed' as ResultMsg
		return
	end
	
	INSERT INTO WarZNewsletterSubscribers (
		email,
		SubscribeDate
	) values (
		@in_email,
		GETDATE()
	)
	
	select 0 as ResultCode
	return

END


GO

-- ----------------------------
-- Procedure structure for BN_WarZ_PreorderChangeEmail
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_PreorderChangeEmail]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_PreorderChangeEmail]
	@in_OrderID varchar(128),
	@in_emailOld varchar(128),
	@in_emailNew varchar(128)
AS
BEGIN
	SET NOCOUNT ON;
	
	declare @email varchar(128)
	declare @Serial1 varchar(128)
	declare @Serial2 varchar(128)
	declare @Serial3 varchar(128)
	declare @Serial4 varchar(128)
	select 
		@email=email,
		@Serial1=Serial1, 
		@Serial2=Serial2, 
		@Serial3=Serial3,
		@Serial4=Serial4		
	from WarZPreorders where OrderID=@in_OrderID
	if(@@ROWCOUNT = 0) begin
		select 1 as ResultCode, 'no order found' as ResultMsg
		return
	end
	
	if(@email <> @in_emailOld) begin
		select 1 as ResultCode, 'order email does not match with old email' as ResultMsg
		return
	end

	update WarZPreorders set email=@in_emailNew where OrderId=@in_OrderID

	if(@Serial1 <> '') update WarZSerials set email=@in_emailNew where SerialKey=@Serial1
	if(@Serial2 <> '') update WarZSerials set email=@in_emailNew where SerialKey=@Serial2
	if(@Serial3 <> '') update WarZSerials set email=@in_emailNew where SerialKey=@Serial3
	if(@Serial4 <> '') update WarZSerials set email=@in_emailNew where SerialKey=@Serial4
	
	declare @msg varchar(1024) = 'email changed for serials<br>' + @Serial1 + '<br>' + @Serial2 + '<br>' + @Serial3 + '<br>' + @Serial4

	select 0 as ResultCode, @msg as ResultMsg

	return
END


GO

-- ----------------------------
-- Procedure structure for BN_WarZ_PreorderGetAll
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_PreorderGetAll]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_PreorderGetAll]
	@in_email varchar(128)
AS
BEGIN
	SET NOCOUNT ON;
	
	-- used in account.thewarz.com/preorder/ordercheck-exec.php
	
	select * from WarZPreorders where email=@in_email
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_PreorderGetAll2
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_PreorderGetAll2]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_PreorderGetAll2]
	@in_order varchar(128)
AS
BEGIN
	SET NOCOUNT ON;
	
	-- used in account.thewarz.com/admin/ordercheck-exec.php
	
	select * from WarZPreorders where email=@in_order or OrderID=@in_order
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_PreorderGetAll3
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_PreorderGetAll3]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_PreorderGetAll3]
	@in_order varchar(128)
AS
BEGIN
	SET NOCOUNT ON;
	
	-- used in account.thewarz.com/admin/ordercheck-exec.php
	
	select * from dbo.WarZDeletedOrders where email=@in_order or OrderID=@in_order
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_PreorderRegister
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_PreorderRegister]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_PreorderRegister]
	@in_Method varchar(32),
	@in_Amount float,
	@in_OrderID varchar(128),
	@in_ApprovalCode varchar(128),
	@in_email varchar(128),
	@in_AccountType int,
	@in_ReferrerName varchar(128) = '',
	@in_ClientIP varchar(32) = '0.0.0.0'
AS
BEGIN
	SET NOCOUNT ON;

	-- get one master serial
	declare @Serial1 varchar(128) = ''
	exec BN_WarZ_GetSerial @in_email, @in_AccountType, @Serial1 out

	-- note about @in_AccountType	
	-- 0: Legend Account Type – просто три разных категории неограниченных про времени эккаунтов	
	-- 1: Pioneer Account Type
	-- 2: Survivor Account Type
	-- 3: Guest key
	-- 4: $15
	-- 5: $25
	-- 6: $50
	-- RUS
	-- 50: guest
	-- 51: packet0
	-- 52: packet1
	-- 53: packet3
	-- 54: superpack

	-- guest keys
	declare @Serial2 varchar(128) = ''
	declare @Serial3 varchar(128) = ''
	declare @Serial4 varchar(128) = ''

	-- pioneer and legend account types will get 3 guest serials
	--if(@in_AccountType = 0 or @in_AccountType = 1 or (@in_AccountType >= 4 and @in_AccountType <= 6)) begin
	--	exec BN_WarZ_GetSerial @in_email, 3, @Serial2 out
	--	exec BN_WarZ_GetSerial @in_email, 3, @Serial3 out
	--	exec BN_WarZ_GetSerial @in_email, 3, @Serial4 out
	--end

	-- 2013-03-12 - no more guest keys
	--if((@in_AccountType >= 51 and @in_AccountType <= 54)) begin
	--	exec BN_WarZ_GetSerial @in_email, 50, @Serial2 out
	--	exec BN_WarZ_GetSerial @in_email, 50, @Serial3 out
	--	exec BN_WarZ_GetSerial @in_email, 50, @Serial4 out
	--end

	-- store transaction	
	INSERT INTO WarZPreorders (
		OrderDate,
		Method,
		Amount,
		OrderID,
		ApprovalCode,
		email,
		AccountType,
		Serial1,
		Serial2,
		Serial3,
		Serial4,
		ReferrerName,
		ClientIP
	) values (
		GETDATE(),
		@in_Method,
		@in_Amount,
		@in_OrderID,
		@in_ApprovalCode,
		@in_email,
		@in_AccountType,
		@Serial1,
		@Serial2,
		@Serial3,
		@Serial4,
		@in_ReferrerName,
		@in_ClientIP
	)
	
	select 0 as ResultCode

	-- report serials
	select 
		@Serial1 as 'Serial1', 
		@Serial2 as 'Serial2', 
		@Serial3 as 'Serial3', 
		@Serial4 as 'Serial4'

	return
END


GO

-- ----------------------------
-- Procedure structure for BN_WarZ_PreorderRemove
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_PreorderRemove]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_PreorderRemove]
	@in_email varchar(128)
AS
BEGIN
	SET NOCOUNT ON;

	insert into WarZDeletedOrders (
		[Method],
		[Amount],
		[OrderID],
		[ApprovalCode],
		[email],
		[AccountType],
		[Serial1],
		[Serial2],
		[Serial3],
		[Serial4]
	) select 
		[Method],
		[Amount],
		[OrderID],
		[ApprovalCode],
		[email],
		[AccountType],
		[Serial1],
		[Serial2],
		[Serial3],
		[Serial4]
	from WarZPreorders where email=@in_email

	select * from WarZPreorders where email=@in_email
	select * from WarZSerials where IsUsed>0 and email=@in_email

	delete from WarZPreorders where email=@in_email
	delete from WarZSerials where IsUsed>0 and email=@in_email

	return
END


GO

-- ----------------------------
-- Procedure structure for BN_WarZ_PreorderRemoveByOrderId
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_PreorderRemoveByOrderId]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_PreorderRemoveByOrderId]
	@in_OrderId varchar(128)
AS
BEGIN
	SET NOCOUNT ON;

	select * from dbo.WarZPreorders where OrderId=@in_OrderId
	
	-- get order info
	declare @email varchar(255)
	declare @Serial1 varchar(128)
	declare @Serial2 varchar(128)
	declare @Serial3 varchar(128)
	declare @Serial4 varchar(128)
	select @email=email, 
		@Serial1=Serial1, 
		@Serial2=Serial2, 
		@Serial3=Serial3,
		@Serial4=Serial4
	from dbo.WarZPreorders where OrderID=@in_OrderId
	if(@@ROWCOUNT = 0) begin
		return
	end
	
	-- remove preorder
	insert into WarZDeletedOrders (
		[Method],
		[Amount],
		[OrderID],
		[ApprovalCode],
		[email],
		[AccountType],
		[Serial1],
		[Serial2],
		[Serial3],
		[Serial4]
	) select 
		[Method],
		[Amount],
		[OrderID],
		[ApprovalCode],
		[email],
		[AccountType],
		[Serial1],
		[Serial2],
		[Serial3],
		[Serial4]
	from WarZPreorders where OrderID=@in_OrderId

	delete from WarZPreorders where OrderID=@in_OrderId
	
	-- remove keys
	declare @CustomerID int
	select @CustomerID=CustomerID from dbo.WarZSerials where SerialKey=@Serial1
	if(@Serial1 <> '') update dbo.WarZSerials set IsBlocked=1 where SerialKey=@Serial1
	if(@Serial2 <> '') update dbo.WarZSerials set IsBlocked=1 where SerialKey=@Serial2
	if(@Serial3 <> '') update dbo.WarZSerials set IsBlocked=1 where SerialKey=@Serial3
	if(@Serial4 <> '') update dbo.WarZSerials set IsBlocked=1 where SerialKey=@Serial4
	
	-- remove WarZ account if exists. MAKE SURE that bn_api_user is mapped to WarZ DB and have permission to run it
	if(@CustomerID > 0) begin
		select 'Customer Had WarZ Account', @CustomerID
		exec [WarZ].[dbo].WZ_ACCOUNT_DELETE @CustomerID
	end
	
	return
END


GO

-- ----------------------------
-- Procedure structure for BN_WarZ_PreorderRemoveBySerialKey
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_PreorderRemoveBySerialKey]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_PreorderRemoveBySerialKey]
	@in_SerialKey varchar(128)
AS
BEGIN
	SET NOCOUNT ON;

	declare @OrderID varchar(128)
	select @OrderID=OrderID from dbo.WarZPreorders where Serial1=@in_SerialKey
	if(@@ROWCOUNT = 0) begin
		select 'There is no order with that serial'
	end

	exec BN_WarZ_PreorderRemoveByOrderId @OrderID	
	
	return
END


GO

-- ----------------------------
-- Procedure structure for BN_WarZ_SerialCheck
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_SerialCheck]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_SerialCheck]
	@in_SerialKey varchar(128),
	@in_email varchar(128)
AS
BEGIN
	SET NOCOUNT ON;
	
	-- call is always success
	select 0 as ResultCode

	update DBG_SerialChecks set SerialCheck=(SerialCheck+1) where RecordID=1
	
	declare @email varchar(128)
	declare @CustomerID int
	declare @IsUsed int
	declare @IsBlocked int
	declare @SerialType int

	select 
		@email=email,
		@CustomerID=CustomerID,
		@IsUsed=IsUsed,
		@IsBlocked=IsBlocked,
		@SerialType=SerialType
	from WarZSerials where SerialKey=@in_SerialKey
	if(@@ROWCOUNT = 0 or @IsUsed = 0) begin
		select 1 as CheckCode, 'Serial Key is not valid' as CheckMsg, -1 as SerialType
		return
	end

	if(@IsBlocked > 0) begin
		select 5 as CheckCode, 'Your Serial Key is BLOCKED' as CheckMsg, -1 as SerialType
		return
	end
	
	--we do not check for email anymore
	--if(@email <> @in_email) begin
	--	select 2 as CheckCode, 'Serial Key and order email do not match' as CheckMsg, -1 as SerialType
	--	return
	--end
	
	if(@CustomerID > 0) begin
		select 3 as CheckCode, 'You already have The War Z account for this Serial Key' as CheckMsg, -1 as SerialType
		return
	end

	if(@SerialType = 3 or @SerialType = 50) begin
		select 3 as CheckCode, 'Guest keys have been disabled.' as CheckMsg, -1 as SerialType
		return
	end

	select 0 as CheckCode, 'Serial Key is Valid' as CheckMsg, @SerialType as SerialType
	return
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_SerialCheck2
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_SerialCheck2]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_SerialCheck2]
	@in_SerialKey varchar(128),
	@in_email varchar(128)
AS
BEGIN
	SET NOCOUNT ON;
	
	-- temp function used for account site getforumbadge-exec.php

	select * from WarZSerials where SerialKey=@in_SerialKey
	return
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_SerialCheckWithEmail
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_SerialCheckWithEmail]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_SerialCheckWithEmail]
	@in_SerialKey varchar(128),
	@in_email varchar(128),
	@out_CustomerID int out
AS
BEGIN
	SET NOCOUNT ON;
	
	select @out_CustomerID=CustomerID from WarZSerials where SerialKey=@in_SerialKey and email=@in_email
	if(@@ROWCOUNT = 0)
		set @out_CustomerID = 0

	return
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_SerialGetInfo
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_SerialGetInfo]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_SerialGetInfo]
	@in_SerialKey varchar(128),
	@in_email varchar(128),
	@out_ResultCode int out,
	@out_CustomerID int out,
	@out_SerialType int out
AS
BEGIN
	SET NOCOUNT ON;

	-- special case for steam account registration (also in WarZ db)
	if(@in_SerialKey = 'STEAM-226700-FGHYT-AWRTS-HZRTA')
	begin
		set @out_ResultCode = 0
		set @out_CustomerID = 0
		set @out_SerialType = 10
		return 1
	end
	
	declare @CustomerID int
	declare @SerialType int
	
	update DBG_SerialChecks set SerialGetInfo=(SerialGetInfo+1) where RecordID=1

	-- search for valid, used and not blocked serial, ignore email
	select @CustomerID=CustomerID, @SerialType=SerialType
	from WarZSerials where SerialKey=@in_SerialKey and IsUsed>0 and IsBlocked=0
	if(@@ROWCOUNT = 0) begin
		set @out_ResultCode = 1
		return 0
	end
	
	set @out_ResultCode = 0
	set @out_CustomerID = @CustomerID
	set @out_SerialType = @SerialType
				
	return 1
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_SerialRemove
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_SerialRemove]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_SerialRemove]
	@in_SerialKey varchar(128)
AS
BEGIN
	SET NOCOUNT ON;

	-- remove keys
	declare @CustomerID int
	select @CustomerID=CustomerID from dbo.WarZSerials where SerialKey=@in_SerialKey
	if(@@ROWCOUNT=0) begin
		select @in_SerialKey as 'NOT EXISTS'
		return
	end
	delete from dbo.WarZSerials where SerialKey=@in_SerialKey
	
	-- remove WarZ account if exists. MAKE SURE that bn_api_user is mapped to WarZ DB and have permission to run it
	if(@CustomerID > 0) begin
		select 'Customer Had WarZ Account', @CustomerID
		exec [WarZ].[dbo].WZ_ACCOUNT_DELETE @CustomerID
	end
	
	return
END


GO

-- ----------------------------
-- Procedure structure for BN_WarZ_SerialSetCustomerID
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_SerialSetCustomerID]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_SerialSetCustomerID]
	@in_SerialKey varchar(128),
	@in_CustomerID int
AS
BEGIN
	SET NOCOUNT ON;

	-- special case for steam account registration
	if(@in_SerialKey = 'STEAM-226700-FGHYT-AWRTS-HZRTA')
	begin
		return
	end
	
	update dbo.WarZSerials set CustomerID=@in_CustomerID where SerialKey=@in_SerialKey
	return
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_SerialSetForumUser
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_SerialSetForumUser]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_SerialSetForumUser]
	@in_SerialKey varchar(128),
	@in_ForumUser int
AS
BEGIN
	SET NOCOUNT ON;
	
	update WarZSerials set ForumUser=@in_ForumUser where SerialKey=@in_SerialKey	
	return
END

















GO

-- ----------------------------
-- Procedure structure for BN_WarZ_ShowBannedUsers
-- ----------------------------
DROP PROCEDURE [dbo].[BN_WarZ_ShowBannedUsers]
GO
CREATE PROCEDURE [dbo].[BN_WarZ_ShowBannedUsers]
AS
BEGIN
	SET NOCOUNT ON;
	
	select u.AccountStatus,BanTime,BanReason,BanExpireDate,p.OrderID,p.email 
	from WarZ.dbo.UsersData u 
	left join WarZ.dbo.Accounts a on a.CustomerID=u.CustomerID 
	left join breezenet.dbo.WarZSerials s on s.CustomerID=u.CustomerID 
	left join BreezeNet.dbo.WarZPreorders p on p.Serial1=s.SerialKey 
	where u.AccountStatus>=200 and u.AccountType<>3 and u.BanExpireDate > GETDATE()
	order by BanTime desc

END


GO

-- ----------------------------
-- Procedure structure for BN_Xsolla_CancelOrder
-- ----------------------------
DROP PROCEDURE [dbo].[BN_Xsolla_CancelOrder]
GO
CREATE PROCEDURE [dbo].[BN_Xsolla_CancelOrder]
	@in_id varchar(128)
AS
BEGIN
	SET NOCOUNT ON;
	
	declare @x_id int
	declare @x_v1 varchar(255)
	declare @x_datetime varchar(32)
	declare @xOrderDate datetime
	
	select @x_id=x_id,@x_v1=x_v1,@x_datetime=x_datetime,@xOrderDate=xOrderDate from XsollaOrders where x_id=@in_id
	if(@@ROWCOUNT = 0) begin
		select 0 as 'x_id', 'no order' as 'desc'
		return
	end

	-- convert xsolla order to our ourder id
	declare @OrderId varchar(128) = convert(varchar(128), @x_id)

	-- get order info
	declare @email varchar(255)
	declare @Serial1 varchar(128)
	declare @Serial2 varchar(128)
	declare @Serial3 varchar(128)
	declare @Serial4 varchar(128)
	select @email=email, 
		@Serial1=Serial1, 
		@Serial2=Serial2, 
		@Serial3=Serial3,
		@Serial4=Serial4
	from dbo.WarZPreorders where Method='XSOLLA' and OrderID=@OrderId
	if(@@ROWCOUNT = 0) begin
		select 0 as 'x_id', 'no email' as 'desc'
		return
	end
	
	--(not used) do not cancel transaction if serial key was used
	--declare @CustomerID int = 0
	--select @CustomerID=CustomerID from dbo.WarZSerials where SerialKey=@Serial1
	--if(@CustomerID > 0) begin
	--	select 7 as 'x_id', @Serial1 as 'desc'
	--	return
	--end
	
	-- delete from xslla tables
	insert into dbo.XsollaCancels (
		x_id, 
		x_v1, 
		x_datetime,
		xOrderDate,
		xCancelDate
	) values (
		@x_id, 
		@x_v1, 
		@x_datetime,
		@xOrderDate,
		GETDATE()
	)

	delete from XSollaOrders where x_id=@in_id

	-- return id now
	select @x_id as 'x_id', 'Success' as 'desc'
	
	-- actually remove order
	exec BN_WarZ_PreorderRemoveByOrderId @OrderId
	
	return
END


GO

-- ----------------------------
-- Procedure structure for BN_Xsolla_GetOrder
-- ----------------------------
DROP PROCEDURE [dbo].[BN_Xsolla_GetOrder]
GO
CREATE PROCEDURE [dbo].[BN_Xsolla_GetOrder]
	@in_id varchar(128)
AS
BEGIN
	SET NOCOUNT ON;
	
	select * from XsollaOrders x
		left join dbo.WarZPreorders o on o.OrderId=@in_id
		where x_id=@in_id 
	return
END


GO

-- ----------------------------
-- Procedure structure for BN_Xsolla_StoreOrder
-- ----------------------------
DROP PROCEDURE [dbo].[BN_Xsolla_StoreOrder]
GO
CREATE PROCEDURE [dbo].[BN_Xsolla_StoreOrder]
	@in_ClientIP varchar(32),
	@in_id varchar(128),
	@in_v1 varchar(255),
	@in_v2 varchar(200),
	@in_v3 varchar(100),
	@in_amount float,
	@in_currency varchar(32),
	@in_datetime varchar(32),
	@in_o_email varchar(128),	-- order email
	@in_o_AccType int,			-- order account type
	@in_o_RefName varchar(32)	-- order referrer name
AS
BEGIN
	SET NOCOUNT ON;
	
	insert into XsollaOrders (
		xOrderDate,
		xClientIP,
		x_id,
		x_v1, x_v2, x_v3,
		x_amount, x_currency,
		x_datetime
	) values (
		GETDATE(),
		@in_ClientIP,
		@in_id,
		@in_v1, @in_v2, @in_v3,
		@in_amount, @in_currency,
		@in_datetime
	)

	-- output our orderid
	declare @xOrderID int = SCOPE_IDENTITY()
	select @xOrderID as 'xOrderID'
	
	-- register order and output keys info there
	exec BN_WarZ_PreorderRegister
		'XSOLLA',
		@in_amount,
		@in_id,
		'',
		@in_o_email,
		@in_o_AccType,
		@in_o_RefName,
		@in_ClientIP

END


GO

-- ----------------------------
-- Procedure structure for DB_BackupDatabase
-- ----------------------------
DROP PROCEDURE [dbo].[DB_BackupDatabase]
GO
CREATE PROCEDURE [dbo].[DB_BackupDatabase]   
AS  
BEGIN  
	SET NOCOUNT ON;  

	declare @databaseName sysname = 'BreezeNet'
	declare @sqlCommand NVARCHAR(1000)
	declare @dateTime NVARCHAR(20)

	SELECT @dateTime = REPLACE(CONVERT(VARCHAR, GETDATE(),111),'/','') +
	REPLACE(CONVERT(VARCHAR, GETDATE(),108),':','')

	SET @sqlCommand = 'BACKUP DATABASE ' + @databaseName +
		' TO DISK = ''E:\SQL_Backup\' + @databaseName + '_Full_' + @dateTime + '.BAK'''
         
	select @sqlCommand
	EXECUTE sp_executesql @sqlCommand
END


GO

-- ----------------------------
-- Indexes structure for table DBG_SerialChecks
-- ----------------------------

-- ----------------------------
-- Primary Key structure for table DBG_SerialChecks
-- ----------------------------
ALTER TABLE [dbo].[DBG_SerialChecks] ADD PRIMARY KEY ([RecordID])
GO

-- ----------------------------
-- Indexes structure for table WarIncConversionUsers
-- ----------------------------

-- ----------------------------
-- Primary Key structure for table WarIncConversionUsers
-- ----------------------------
ALTER TABLE [dbo].[WarIncConversionUsers] ADD PRIMARY KEY ([WarIncCustomerID])
GO

-- ----------------------------
-- Indexes structure for table WarZNewsletterSubscribers
-- ----------------------------

-- ----------------------------
-- Primary Key structure for table WarZNewsletterSubscribers
-- ----------------------------
ALTER TABLE [dbo].[WarZNewsletterSubscribers] ADD PRIMARY KEY ([email])
GO

-- ----------------------------
-- Indexes structure for table WarZPreorders
-- ----------------------------
CREATE INDEX [IX_WarZPreorders_Serial1] ON [dbo].[WarZPreorders]
([Serial1] ASC) 
GO

-- ----------------------------
-- Indexes structure for table WarZSerials
-- ----------------------------
CREATE INDEX [IX_WarZSerials_CustomerID] ON [dbo].[WarZSerials]
([CustomerID] ASC) 
GO
CREATE INDEX [IX_WarZSerials_IsUsed] ON [dbo].[WarZSerials]
([IsUsed] ASC) 
GO

-- ----------------------------
-- Primary Key structure for table WarZSerials
-- ----------------------------
ALTER TABLE [dbo].[WarZSerials] ADD PRIMARY KEY ([SerialID])
GO

-- ----------------------------
-- Uniques structure for table WarZSerials
-- ----------------------------
ALTER TABLE [dbo].[WarZSerials] ADD UNIQUE ([SerialKey] ASC)
GO

-- ----------------------------
-- Indexes structure for table XsollaCancels
-- ----------------------------

-- ----------------------------
-- Primary Key structure for table XsollaCancels
-- ----------------------------
ALTER TABLE [dbo].[XsollaCancels] ADD PRIMARY KEY ([x_id])
GO

-- ----------------------------
-- Indexes structure for table XsollaOrders
-- ----------------------------

-- ----------------------------
-- Primary Key structure for table XsollaOrders
-- ----------------------------
ALTER TABLE [dbo].[XsollaOrders] ADD PRIMARY KEY ([x_id])
GO
