--insert into Device (device_id, device_name, vref)
--values ('08:D1:F9:E6:FE:5C', 'FJ Destructive Test Device' ,1128);


--ALTER TABLE Device
--ADD device_ip_address VARCHAR(15) NULL;

--UPDATE Device
--SET [device_ip_address] = '192.168.141.212';

select * from Device

select * from SampleResult
order by created_at desc

--UPDATE SampleResult
--SET test_result = 'pass'
--WHERE test_id = '9C3947C8-EDC0-49B0-872D-6106EEB60CB3';

--delete from SampleResult
--Where test_id = 'E7742D61-B788-4917-B75F-8C403CFBEBF4';

--UPDATE SampleResult
--SET test_result = 
--    CASE
--        WHEN max_load_reading < fifth_uts_lbs THEN 'fail'
--        ELSE 'pass'
--    END;

--select operator_first_name from SampleResult


--INSERT INTO SampleResult(
--    device_id,
--    operator_first_name,
--    project_id,
--    panel_id,
--    shift_id,
--    sample_date,
--    sample_time,
--    specie,
--    grade,
--    dimension,
--    mc_right,
--    mc_left,
--    max_psi_reading,
--    max_load_reading,
--    wood_failure_mode,
--    min_ft_psi,
--    fifth_ft_psi,
--    min_uts_lbs,
--    fifth_uts_lbs,
--    adhesive_application,
--    squeeze_out,
--    adhesive_batch_test_result,
--    finished_joint_appearance,
--    positioning_alignment,
--    test_result
--)
--SELECT
--    device_id,
--    operator_first_name,
--    project_id,
--    panel_id,
--    shift_id,
--    sample_date,
--    sample_time,
--    specie,
--    grade,
--    dimension,
--    mc_right,
--    mc_left,
--    max_psi_reading,
--    max_load_reading,
--    wood_failure_mode,
--    min_ft_psi,
--    fifth_ft_psi,
--    min_uts_lbs,
--    fifth_uts_lbs,
--    adhesive_application,
--    squeeze_out,
--    adhesive_batch_test_result,
--    finished_joint_appearance,
--    positioning_alignment,
--    test_result
--FROM SampleResult
--WHERE test_id = 'E7742D61-B788-4917-B75F-8C403CFBEBF4';


--UPDATE SampleResult
--SET test_result = 'pass'
--WHERE test_result IS NULL OR test_result = '';

