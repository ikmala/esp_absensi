-- phpMyAdmin SQL Dump
-- version 5.2.1
-- https://www.phpmyadmin.net/
--
-- Host: 127.0.0.1
-- Generation Time: May 20, 2025 at 07:39 PM
-- Server version: 10.4.32-MariaDB
-- PHP Version: 8.2.12

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `absensi`
--

-- --------------------------------------------------------

--
-- Table structure for table `absensi`
--

CREATE TABLE `absensi` (
  `IDabsensi` int(11) NOT NULL,
  `UUIDguru` int(11) NOT NULL,
  `Tanggal` date NOT NULL,
  `WaktuMasuk` time DEFAULT NULL,
  `WaktuKeluar` time DEFAULT NULL,
  `Keterangan` text DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- --------------------------------------------------------

--
-- Table structure for table `admin`
--

CREATE TABLE `admin` (
  `IDAdmin` int(11) NOT NULL,
  `Username` varchar(255) NOT NULL,
  `Password` varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `admin`
--

INSERT INTO `admin` (`IDAdmin`, `Username`, `Password`) VALUES
(1, 'admin', '1234');

-- --------------------------------------------------------

--
-- Table structure for table `guru`
--

CREATE TABLE `guru` (
  `IDguru` int(11) NOT NULL,
  `UUid` varchar(255) NOT NULL,
  `Nama` varchar(255) NOT NULL,
  `LakiPerempuan` varchar(255) NOT NULL,
  `TTL` varchar(255) NOT NULL,
  `NUPTK` varchar(100) NOT NULL,
  `JabatanID` int(11) NOT NULL,
  `TKT` varchar(100) NOT NULL,
  `Jurusan` varchar(255) NOT NULL,
  `TahunLulus` varchar(255) NOT NULL,
  `TMT` varchar(255) NOT NULL,
  `StatusIndukNonInduk` varchar(255) NOT NULL,
  `Alamat` text NOT NULL,
  `NoHp` varchar(20) NOT NULL,
  `DevisiID` int(11) NOT NULL,
  `StatusGuruID` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- --------------------------------------------------------

--
-- Table structure for table `jabatan`
--

CREATE TABLE `jabatan` (
  `IDjabatan` int(11) NOT NULL,
  `Nama` varchar(255) NOT NULL,
  `Status` varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `jabatan`
--

INSERT INTO `jabatan` (`IDjabatan`, `Nama`, `Status`) VALUES
(1, 'Direktur', 'Aktif');

-- --------------------------------------------------------

--
-- Table structure for table `kelas`
--

CREATE TABLE `kelas` (
  `IDkelas` int(11) NOT NULL,
  `Nama` varchar(255) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `kelas`
--

INSERT INTO `kelas` (`IDkelas`, `Nama`) VALUES
(2, 'X');

-- --------------------------------------------------------

--
-- Table structure for table `organisasidevisi`
--

CREATE TABLE `organisasidevisi` (
  `IDorganisasi` int(11) NOT NULL,
  `Nama` varchar(255) NOT NULL,
  `Status` varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `organisasidevisi`
--

INSERT INTO `organisasidevisi` (`IDorganisasi`, `Nama`, `Status`) VALUES
(2, 'Robotika', 'Aktif');

-- --------------------------------------------------------

--
-- Table structure for table `statusguru`
--

CREATE TABLE `statusguru` (
  `IDstatus` int(11) NOT NULL,
  `Nama` varchar(255) NOT NULL,
  `Status` varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `statusguru`
--

INSERT INTO `statusguru` (`IDstatus`, `Nama`, `Status`) VALUES
(1, 'Honorer', 'Aktif');

-- --------------------------------------------------------

--
-- Table structure for table `subkelas`
--

CREATE TABLE `subkelas` (
  `IDsubKelas` int(11) NOT NULL,
  `SubKelas` varchar(255) NOT NULL,
  `WaliSubKelas` varchar(255) NOT NULL,
  `KelasID` int(11) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- --------------------------------------------------------

--
-- Table structure for table `tahunajaran`
--

CREATE TABLE `tahunajaran` (
  `IDthnAjaran` int(11) NOT NULL,
  `Nama` varchar(255) NOT NULL,
  `Status` varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Dumping data for table `tahunajaran`
--

INSERT INTO `tahunajaran` (`IDthnAjaran`, `Nama`, `Status`) VALUES
(1, '2024/2025', 'Aktif');

--
-- Indexes for dumped tables
--

--
-- Indexes for table `absensi`
--
ALTER TABLE `absensi`
  ADD PRIMARY KEY (`IDabsensi`),
  ADD KEY `fk_guru_absensi` (`UUIDguru`);

--
-- Indexes for table `admin`
--
ALTER TABLE `admin`
  ADD PRIMARY KEY (`IDAdmin`);

--
-- Indexes for table `guru`
--
ALTER TABLE `guru`
  ADD PRIMARY KEY (`IDguru`),
  ADD KEY `JabatanID` (`JabatanID`),
  ADD KEY `DevisiID_3` (`DevisiID`);

--
-- Indexes for table `jabatan`
--
ALTER TABLE `jabatan`
  ADD PRIMARY KEY (`IDjabatan`);

--
-- Indexes for table `kelas`
--
ALTER TABLE `kelas`
  ADD PRIMARY KEY (`IDkelas`);

--
-- Indexes for table `organisasidevisi`
--
ALTER TABLE `organisasidevisi`
  ADD PRIMARY KEY (`IDorganisasi`);

--
-- Indexes for table `statusguru`
--
ALTER TABLE `statusguru`
  ADD PRIMARY KEY (`IDstatus`);

--
-- Indexes for table `subkelas`
--
ALTER TABLE `subkelas`
  ADD PRIMARY KEY (`IDsubKelas`),
  ADD KEY `KelasID` (`KelasID`);

--
-- Indexes for table `tahunajaran`
--
ALTER TABLE `tahunajaran`
  ADD PRIMARY KEY (`IDthnAjaran`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `absensi`
--
ALTER TABLE `absensi`
  MODIFY `IDabsensi` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=10;

--
-- AUTO_INCREMENT for table `admin`
--
ALTER TABLE `admin`
  MODIFY `IDAdmin` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=2;

--
-- AUTO_INCREMENT for table `guru`
--
ALTER TABLE `guru`
  MODIFY `IDguru` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=12;

--
-- AUTO_INCREMENT for table `jabatan`
--
ALTER TABLE `jabatan`
  MODIFY `IDjabatan` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=3;

--
-- AUTO_INCREMENT for table `kelas`
--
ALTER TABLE `kelas`
  MODIFY `IDkelas` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=3;

--
-- AUTO_INCREMENT for table `organisasidevisi`
--
ALTER TABLE `organisasidevisi`
  MODIFY `IDorganisasi` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=3;

--
-- AUTO_INCREMENT for table `statusguru`
--
ALTER TABLE `statusguru`
  MODIFY `IDstatus` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=2;

--
-- AUTO_INCREMENT for table `subkelas`
--
ALTER TABLE `subkelas`
  MODIFY `IDsubKelas` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=9;

--
-- AUTO_INCREMENT for table `tahunajaran`
--
ALTER TABLE `tahunajaran`
  MODIFY `IDthnAjaran` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=3;

--
-- Constraints for dumped tables
--

--
-- Constraints for table `absensi`
--
ALTER TABLE `absensi`
  ADD CONSTRAINT `fk_guru_absensi` FOREIGN KEY (`UUIDguru`) REFERENCES `guru` (`IDguru`);

--
-- Constraints for table `guru`
--
ALTER TABLE `guru`
  ADD CONSTRAINT `guru_ibfk_1` FOREIGN KEY (`JabatanID`) REFERENCES `jabatan` (`IDjabatan`);

--
-- Constraints for table `subkelas`
--
ALTER TABLE `subkelas`
  ADD CONSTRAINT `subkelas_ibfk_1` FOREIGN KEY (`KelasID`) REFERENCES `kelas` (`IDkelas`);
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
